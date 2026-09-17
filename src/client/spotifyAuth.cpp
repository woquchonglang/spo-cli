module;
#include <cstddef>
#include <sys/socket.h>
#include <stdlib.h>
#include <wordexp.h>
module spotifyAuth;

import nlohmann.json;
import httplib;
import std;

namespace fs = std::filesystem;

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

static std::string generateRandomString(size_t length);

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

SpotifyAuth::SpotifyAuth(const Config &config) : config(config) {
    fs::path cache_path = expand_shell_path(config.cache_path);
    cache_file = cache_path / "token.json";
}


void SpotifyAuth::saveTokenCache(const TokenCache &cache,
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

std::optional<TokenCache>
SpotifyAuth::loadTokenCache(const fs::path &cachePath) {
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

void SpotifyAuth::exchangeCodeForToken(std::string_view code) {
    httplib::Client client("https://accounts.spotify.com");
    client.set_address_family(AF_INET);
    std::string body =
            "code=" + httplib::encode_uri_component(code.data()) +
            "&redirect_uri=" +
            httplib::encode_uri_component(config.login_redirect_url) +
            "&grant_type=authorization_code";
    client.set_basic_auth(config.client_id, config.client_secret);
    auto res = client.Post("/api/token", body,
                           "application/x-www-form-urlencoded");

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
        *this->access_token = response["access_token"];
        this->refresh_token = response["refresh_token"];
        this->expires_in = response["expires_in"];

        TokenCache cache{ .access_token = *this->access_token,
                          .refresh_token = this->refresh_token,
                          .expires_in = this->expires_in,
                          .token_acquire_time =
                                  std::chrono::system_clock::now() };
        saveTokenCache(cache, cache_file);
    } else {
        std::cerr << "HTTP request failed, error code: "
                  << static_cast<int>(res.error()) << std::endl;
    }
}

/*
 * https://developer.spotify.com/documentation/web-api/tutorials/code-flow
 */
bool SpotifyAuth::login() {
    auto client_id = config.client_id;
    auto redirect_url = config.login_redirect_url;

    if (loadTokenCache(cache_file).has_value()) {
        auto cache = loadTokenCache(cache_file).value();
        auto now = std::chrono::system_clock::now();
        if ((now - cache.token_acquire_time) <
            std::chrono::seconds(cache.expires_in)) {
            *this->access_token = cache.access_token;
            this->refresh_token = cache.refresh_token;
            this->expires_in = cache.expires_in;
            Timer::instance().add_task_after(std::chrono::seconds(expires_in),
                                             [this]() {
                                                 refreshAccessToken();
                                             });
            return true;
        }
    }

    std::atomic<bool> auth_success{ false };
    httplib::Server app;
    app.Get("/login", [&](const httplib::Request &, httplib::Response &res) {
        std::string state = generateRandomString(16);

        auto scopes_to_string = [](const std::vector<std::string> &scopes) {
            std::string result;
            for (size_t i = 0; i < scopes.size(); ++i) {
                if (i != 0) {
                    result += " ";
                }
                result += scopes[i];
            }
            return result;
        };

        // std::string scope = "user-read-private user-read-email user-top-read";
        std::string scope = scopes_to_string(scopes);

        std::ostringstream _redirect_url;
        _redirect_url << "https://accounts.spotify.com/authorize?"
                      << "response_type=code&"
                      << "client_id=" << client_id << "&"
                      << "scope=" << scope << "&"
                      << "redirect_uri=" << redirect_url << "&"
                      << "state=" << state;

        res.set_redirect(_redirect_url.str());
    });

    app.Get("/callback", [&](const httplib::Request &req,
                             httplib::Response &res) {
        auto code = req.get_param_value("code");
        auto state = req.get_param_value("state");

        if (state.empty()) {
            res.status = 400;
            res.set_content("State mismatch", "text/plain");
            return;
        }

        exchangeCodeForToken(code);
        Timer::instance().add_task_after(std::chrono::seconds(expires_in),
                                         [this]() { refreshAccessToken(); });

        res.set_content("Successfully authenticated.", "text/plain");

        if (app.is_running())
            app.stop();

        auth_success = true;
    });

    std::println("login to: http://127.0.0.1:8989/login");

    system("xdg-open http://127.0.0.1:8989/login");

    app.listen("127.0.0.1", 8989);

    int wait_count = 0;
    while (!auth_success && wait_count < 300) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        wait_count++;
    }

    return auth_success;
}

void SpotifyAuth::refreshAccessToken() {
    httplib::Client client("https://accounts.spotify.com");
    client.set_address_family(AF_INET);
    std::string body =
            "grant_type=refresh_token&refresh_token=" + refresh_token;
    client.set_basic_auth(config.client_id, config.client_secret);
    auto res = client.Post("/api/token", body,
                           "application/x-www-form-urlencoded");

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
        *this->access_token = response["access_token"];
        this->expires_in = response["expires_in"];
        // sometimes Spotify may return a new refresh token, so we need to update it if it's present
        this->refresh_token =
                response.value("refresh_token", this->refresh_token);
        Timer::instance().add_task_after(std::chrono::seconds(expires_in),
                                         [this]() { refreshAccessToken(); });
    } else {
        std::cerr << "HTTP request failed, error code: "
                  << static_cast<int>(res.error()) << std::endl;
    }
}

// https://generate-random.org/strings/cpp
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
