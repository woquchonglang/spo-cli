module;
#include <cstdlib>
#include <cstddef>
#include <wordexp.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <skyr/url.hpp>
module spotifyAuthPKCE;

import http.client;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
namespace fs = std::filesystem;

// Code Verifier, refer to https://generate-random.org/strings/cpp
std::string generateRandomString(size_t length) {
    const std::string ALPHANUMERIC =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, ALPHANUMERIC.size() - 1);
    std::string random_string;
    random_string.reserve(length);
    for (int i = 0; i < length; ++i) {
        random_string += ALPHANUMERIC[distribution(generator)];
    }
    return random_string;
}

// Code Challenge
std::vector<uint8_t> sha256(const std::string &plain) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const unsigned char *>(plain.data()), plain.size(),
           hash.data());
    return hash;
}

// base64url encode
std::string base64encode(const std::vector<uint8_t> &hash) {
    std::string b64;
    b64.resize(4 * ((SHA256_DIGEST_LENGTH + 2) / 3));
    int len = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(b64.data()),
                              hash.data(), static_cast<int>(hash.size()));
    b64.resize(len);
    for (char &c : b64) {
        if (c == '+')
            c = '-';
        else if (c == '/')
            c = '_';
    }
    while (!b64.empty() && b64.back() == '=')
        b64.pop_back();
    return b64;
}

std::string expand_shell_path(const std::string &raw_path) {
    wordexp_t expanded_result;
    if (wordexp(raw_path.c_str(), &expanded_result, 0) == 0) {
        std::string full_path(expanded_result.we_wordv[0]);
        return full_path;
    }
    wordfree(&expanded_result);
    std::cerr << "Failed to expand path: " << raw_path << std::endl;
    return raw_path;
}

std::string uri_encode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(c);
        }
    }
    return escaped.str();
}

std::string scopes_to_string(const std::vector<std::string> &scopes) {
    std::string result;
    for (size_t i = 0; i < scopes.size(); ++i) {
        if (i != 0) {
            result += " ";
        }
        result += scopes[i];
    }
    return result;
};

std::string extract_code(const std::string &target) {
    auto pos = target.find("code=");
    if (pos == std::string::npos)
        return "";
    pos += 5;
    auto end = target.find('&', pos);
    if (end == std::string::npos)
        end = target.size();
    return target.substr(pos, end - pos);
}

SpotifyAuthPKCE::SpotifyAuthPKCE(boost::asio::io_context &ioc,
                                 const Config &config)
        : config(config), ioc(ioc), ctx(ssl::context::tls_client) {
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);
    fs::path cache_path = expand_shell_path(config.cache_path);
    cache_file = cache_path / "token.json";
}

SpotifyAuthPKCE::~SpotifyAuthPKCE() {}

unsigned short parse_port_from_url(const std::string &url) {
    auto pos = url.rfind(':');
    if (pos == std::string::npos) {
        if (url.starts_with("https://"))
            return 443;
        return 80;
    }
    auto end = url.find('/', pos);
    std::string port_str = url.substr(pos + 1, end - pos - 1);
    return static_cast<unsigned short>(std::stoi(port_str));
}

class CallbackServer {
public:
    CallbackServer(boost::asio::ip::tcp::acceptor acceptor)
            : acceptor_(std::move(acceptor)) {}

    boost::asio::awaitable<std::string> wait_for_code() {
        auto socket =
                co_await acceptor_.async_accept(boost::asio::use_awaitable);

        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> req;
        auto [ec, n] = co_await boost::beast::http::async_read(
                socket, buffer, req,
                boost::asio::as_tuple(boost::asio::use_awaitable));
        if (ec) {
            std::cerr << "read error: " << ec.message() << std::endl;
            co_return "";
        }

        std::string target = req.target();
        std::string code = extract_code(target);

        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::ok, req.version()
        };
        res.set(boost::beast::http::field::content_type, "text/html");
        res.body() = "<html><body><h1>Authorization Successful</h1>"
                     "<p>You can close this window.</p></body></html>";
        res.prepare_payload();
        co_await boost::beast::http::async_write(socket, res,
                                                 boost::asio::use_awaitable);

        co_return code;
    }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
};


boost::asio::awaitable<CallbackServer>
start_callback_server(unsigned short port) {
    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::ip::tcp::acceptor acceptor(
            executor,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    co_return CallbackServer(std::move(acceptor));
}

boost::asio::awaitable<bool> SpotifyAuthPKCE::login() {
    codeVerifier = generateRandomString(64);
    auto hashed = sha256(codeVerifier);
    auto codeChallenge = base64encode(hashed);
    auto client_id = config.client_id;
    auto redirect_url = config.login_redirect_url;
    auto scope = scopes_to_string(scopes);

    cache = loadTokenCache(cache_file);
    if (cache.has_value()) {
        auto now = std::chrono::system_clock::now();
        if ((now - cache.value().token_acquire_time) <
            std::chrono::seconds(cache.value().expires_in)) {
            *this->access_token = cache.value().access_token;
            this->refresh_token = cache.value().refresh_token;
            this->expires_in = cache.value().expires_in;
            boost::asio::co_spawn(
                    ioc,
                    [this]() -> boost::asio::awaitable<void> {
                        co_await startRefreshTimer();
                    },
                    boost::asio::detached);
            co_return true;
        }
    }

    std::string auth_url = "https://accounts.spotify.com/authorize?"
                           "response_type=code"
                           "&client_id=" +
                           client_id + "&scope=" + uri_encode(scope) +
                           "&code_challenge_method=S256"
                           "&code_challenge=" +
                           codeChallenge +
                           "&redirect_uri=" + uri_encode(redirect_url);

    auto port = parse_port_from_url(config.login_redirect_url);
    auto server = co_await start_callback_server(port);

    std::string cmd = "xdg-open '" + auth_url + "'";
    std::system((cmd + " &").c_str());

    std::string code = co_await server.wait_for_code();

    std::string body = "client_id=" + uri_encode(client_id) +
                       "&grant_type=authorization_code" +
                       "&code=" + uri_encode(code) +
                       "&redirect_uri=" + uri_encode(redirect_url) +
                       "&code_verifier=" + uri_encode(codeVerifier);

    auto response = co_await async_http_post("accounts.spotify.com",
                                             "/api/token", body);

    if (!response.is_null()) {
        *this->access_token = response["access_token"];
        this->refresh_token = response["refresh_token"];
        this->expires_in = response["expires_in"];

        TokenCache cache{ .access_token = *this->access_token,
                          .refresh_token = this->refresh_token,
                          .expires_in = this->expires_in,
                          .token_acquire_time =
                                  std::chrono::system_clock::now() };
        saveTokenCache(cache, cache_file);
        boost::asio::co_spawn(
                ioc,
                [this]() -> boost::asio::awaitable<void> {
                    co_await startRefreshTimer();
                },
                boost::asio::detached);
        co_return true;
    }
    co_return false;
}

boost::asio::awaitable<void> SpotifyAuthPKCE::startRefreshTimer() {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    for (;;) {
        auto now = std::chrono::system_clock::now();
        auto delay = now - cache.value().token_acquire_time;
        timer.expires_after(
                std::chrono::duration_cast<std::chrono::milliseconds>(delay));
        auto [ec] = co_await timer.async_wait(
                boost::asio::as_tuple(boost::asio::use_awaitable));
        if (ec)
            co_return;
        co_await refreshAccessToken();
    }
    co_return;
}

boost::asio::awaitable<void> SpotifyAuthPKCE::refreshAccessToken() {
    auto response = co_await async_http_refreshtoken_post();
    if (!response.is_null()) {
        *this->access_token = response["access_token"];
        this->expires_in = response["expires_in"];
        // sometimes Spotify may return a new refresh token, so we need to update it if it's present
        this->refresh_token =
                response.value("refresh_token", this->refresh_token);

        cache = { .access_token = *this->access_token,
                  .refresh_token = this->refresh_token,
                  .expires_in = this->expires_in,
                  .token_acquire_time = std::chrono::system_clock::now() };
        saveTokenCache(cache.value(), cache_file);
    }
    co_return;
}

std::optional<TokenCache>
SpotifyAuthPKCE::loadTokenCache(const fs::path &cachePath) {
    if (!fs::exists(cachePath))
        return std::nullopt;
    std::ifstream f(cachePath);
    if (!f.is_open())
        return std::nullopt;
    nlohmann::json j = nlohmann::json::parse(f);

    TokenCache cache;
    j.at("access_token").get_to(cache.access_token);
    j.at("refresh_token").get_to(cache.refresh_token);
    j.at("expires_in").get_to(cache.expires_in);
    long long ts = j.at("token_acquire_time").get<long long>();
    cache.token_acquire_time =
            std::chrono::system_clock::time_point(std::chrono::seconds{ ts });
    return cache;
}

namespace std::chrono {
void to_json(nlohmann::json &j, const system_clock::time_point &tp) {
    auto sec = duration_cast<seconds>(tp.time_since_epoch());
    j = sec.count();
}

void from_json(const nlohmann::json &j, system_clock::time_point &tp) {
    long long val = j.get<long long>();
    tp = system_clock::time_point(seconds{ val });
}
}

void SpotifyAuthPKCE::saveTokenCache(const TokenCache &cache,
                                     const fs::path &cache_path) {
    fs::create_directories(cache_path.parent_path());
    nlohmann::json j;
    j["access_token"] = cache.access_token;
    j["refresh_token"] = cache.refresh_token;
    j["expires_in"] = cache.expires_in;
    j["token_acquire_time"] = cache.token_acquire_time;

    std::ofstream f(cache_path);
    if (f.is_open())
        f << j.dump(4);
    f.close();
}

boost::asio::awaitable<nlohmann::json>
SpotifyAuthPKCE::async_http_post(const std::string &host,
                                 const std::string &target,
                                 const std::string &body) {
    http::request<http::string_body> req{ http::verb::post, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::content_type, "application/x-www-form-urlencoded");
    req.set(http::field::user_agent, "Beast/1.0");
    req.body() = std::move(body);
    req.prepare_payload();

    auto res = co_await client_read(host, "443", ctx, req);
    auto result = res.result_int();

    nlohmann::json response;
    if (result == 200) {
        response = nlohmann::json::parse(
                beast::buffers_to_string(res.body().data()));
    } else {
        std::cerr << "http post error: " << result << " "
                  << beast::buffers_to_string(res.body().data()) << std::endl;
    }
    co_return response;
}

net::awaitable<nlohmann::json> SpotifyAuthPKCE::async_http_refreshtoken_post() {
    std::string body = "grant_type=refresh_token"
                       "&refresh_token=" +
                       uri_encode(refresh_token) +
                       "&client_id=" + uri_encode(config.client_id);
    http::request<http::string_body> req{ http::verb::post, "/api/token", 11 };
    req.set(http::field::host, "accounts.spotify.com");
    req.set(http::field::content_type, "application/x-www-form-urlencoded");
    req.body() = body;
    req.prepare_payload();
    auto res = co_await client_read("accounts.spotify.com", "443", ctx, req);
    auto result = res.result_int();
    nlohmann::json response;
    if (result == 200) {
        response = nlohmann::json::parse(
                beast::buffers_to_string(res.body().data()));
    } else
        printf("http post error");
    co_return response;
}
