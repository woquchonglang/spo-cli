module;
#include <sys/socket.h>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
module spotifyWebAPI;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

struct APIImpl {
    APIImpl() : work(boost::asio::make_work_guard(ioc)) {
        asio_thread = std::thread([this]() { ioc.run(); });
    }
    ~APIImpl() {
        ioc.stop();
        if (asio_thread.joinable()) {
            asio_thread.join();
        }
    }

    boost::asio::io_context ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    std::thread asio_thread;
};

template <typename T> T parse_response(const nlohmann::json &response) { return T{}; }

template <> UserProfile parse_response<UserProfile>(const nlohmann::json &response) {
    UserProfile profile;
    profile.account_id = response["account_id"];
    profile.country = response["country"];
    profile.displayName = response["display_name"];
    profile.email = response["email"];
    profile.product = response["product"];
    auto &images = response["images"];
    if (images.is_array() && !images.empty() && !images[0].is_null()) {
        profile.image.url = response["images"][0]["url"];
        profile.image.height = response["images"][0]["height"];
        profile.image.width = response["images"][0]["width"];
    }
    return profile;
}

template <> UserTopArtistsData parse_response<UserTopArtistsData>(const nlohmann::json &response) {
    UserTopArtistsData data;
    for (const auto &artist : response["items"]) {
        data.names.push_back(artist["name"]);
        data.uri.push_back(artist["uri"]);
        auto &images = artist["images"];
        if (images.is_array() && !images.empty() && !images[0].is_null()) {
            Image img;
            img.url = artist["images"][0]["url"];
            img.height = artist["images"][0]["height"];
            img.width = artist["images"][0]["width"];
            data.image.largeImage.push_back(img);
        }
        if (images.is_array() && !images.empty() && !images[1].is_null()) {
            Image img;
            img.url = artist["images"][1]["url"];
            img.height = artist["images"][1]["height"];
            img.width = artist["images"][1]["width"];
            data.image.mediumImage.push_back(img);
        }
        if (images.is_array() && !images.empty() && !images[2].is_null()) {
            Image img;
            img.url = artist["images"][2]["url"];
            img.height = artist["images"][2]["height"];
            img.width = artist["images"][2]["width"];
            data.image.smallImage.push_back(img);
        }
    }
    return data;
}

template <> UserTopTracksData parse_response<UserTopTracksData>(const nlohmann::json &response) {
    UserTopTracksData data;
    for (const auto &album : response["items"]) {
        data.tracks.push_back(album["name"]);
    }
    return data;
}

template <typename T>
net::awaitable<T> async_http_get(const std::string &host, const std::string &target, const std::string &token) {
    ssl::context ctx(ssl::context::tlsv13_client);
    ctx.set_default_verify_paths();

    auto executor = co_await net::this_coro::executor;
    beast::ssl_stream<beast::tcp_stream> stream(executor, ctx);

    tcp::resolver resolver(executor);
    auto const results = co_await resolver.async_resolve(host, "443", net::use_awaitable);

    co_await beast::get_lowest_layer(stream).async_connect(results, net::use_awaitable);
    co_await stream.async_handshake(ssl::stream_base::client, net::use_awaitable);

    http::request<http::string_body> req{ http::verb::get, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::user_agent, "Beast/1.0");

    co_await http::async_write(stream, req, net::use_awaitable);

    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    co_await http::async_read(stream, buffer, res, net::use_awaitable);

    if (res.result_int() == 200) {
        nlohmann::json response = nlohmann::json::parse(beast::buffers_to_string(res.body().data()));
        co_return parse_response<T>(response);
    }

    beast::error_code ec;
    beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_both, ec);
    co_return T{};
}


SpotifyWebAPI::SpotifyWebAPI(std::shared_ptr<std::string> accessToken, std::shared_ptr<SpotifyData> data)
        : accessToken(accessToken), spotifyData(data), apiImpl(std::make_unique<APIImpl>()) {};

SpotifyWebAPI::~SpotifyWebAPI() {}

void SpotifyWebAPI::updateAccessToken(std::shared_ptr<std::string> token) {
    spotifyData->isLogined.write().value = true;
    accessToken = token;
}

void SpotifyWebAPI::getUserProfile(void (*cb)()) {
    net::co_spawn(
            apiImpl->ioc,
            [this, cb]() -> boost::asio::awaitable<void> {
                try {
                    auto profile =
                            co_await async_http_get<UserProfile>("api.spotify.com", "/v1/me", *accessToken.get());

                    spotifyData->userProfile.write().value = profile;

                    cb();

                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                }
            },
            boost::asio::detached);
}

/*
 * Get the current user’s top artists based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
void SpotifyWebAPI::getUserTopArtists(void (*cb)(), const std::string &timeRange, int limit, int offset) {
    std::string url = "/v1/me/top/artists";
    net::co_spawn(
            apiImpl->ioc,
            [this, cb, url]() -> boost::asio::awaitable<void> {
                try {
                    auto artists =
                            co_await async_http_get<UserTopArtistsData>("api.spotify.com", url, *accessToken.get());

                    spotifyData->topArtists.write().value = artists;

                    cb();

                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                }
            },
            boost::asio::detached);
}

/*
 * Get the current user’s top tracks based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
void SpotifyWebAPI::getUserTopTracks(void (*cb)(), const std::string &timeRange, int limit, int offset) {
    std::string url = "/v1/me/top/tracks?time_range=" + timeRange + "&limit=" + std::to_string(limit) +
                      "&offset=" + std::to_string(offset);
    net::co_spawn(
            apiImpl->ioc,
            [this, cb, url]() -> boost::asio::awaitable<void> {
                try {
                    auto tracks =
                            co_await async_http_get<UserTopTracksData>("api.spotify.com", url, *accessToken.get());

                    spotifyData->topTracks.write().value = tracks;

                    cb();

                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                }
            },
            boost::asio::detached);
}

