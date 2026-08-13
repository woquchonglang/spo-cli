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

net::awaitable<void> async_http_get(const std::string &host, const std::string &target) {
    ssl::context ctx(ssl::context::tlsv13_client);
    ctx.set_default_verify_paths();

    auto executor = co_await net::this_coro::executor;
    beast::ssl_stream<beast::tcp_stream> stream(executor, ctx);

    tcp::resolver resolver(executor);
    auto const results = co_await resolver.async_resolve(host, "http", net::use_awaitable);

    co_await beast::get_lowest_layer(stream).async_connect(results, net::use_awaitable);

    http::request<http::string_body> req{ http::verb::get, target, 11 };
    req.set(http::field::host, host);
    co_await http::async_write(stream, req, net::use_awaitable);

    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    co_await http::async_read(stream, buffer, res, net::use_awaitable);

    std::cout << "Async Response: " << res.result_int() << " " << res.reason() << "\n";
    std::cout << "Body size: " << res.body().size() << " bytes\n";

    beast::error_code ec;
    beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_both, ec);
}


SpotifyWebAPI::SpotifyWebAPI(std::shared_ptr<std::string> accessToken) : accessToken(accessToken) {};

UserProfile SpotifyWebAPI::getUserProfile() {
    httplib::Client client("https://api.spotify.com");
    client.set_address_family(AF_INET);
    std::string authHeader = "Bearer " + *accessToken;
    httplib::Headers headers = { { "Authorization", authHeader } };

    auto res = client.Get("/v1/me", headers);

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
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
    } else {
        std::cerr << "HTTP request failed, error code: " << static_cast<int>(res.error()) << std::endl;
        return {};
    }
}

UserProfile SpotifyWebAPI::async_getUserProfile() {
    net::io_context ioc;
    net::co_spawn(ioc, async_http_get("https://api.spotify.com", "/v1/me"), net::detached);
    ioc.run();
}

/*
 * Get the current user’s top artists based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
UserTopArtistsData SpotifyWebAPI::getUserTopArtists(const std::string &timeRange, int limit, int offset) {
    httplib::Client client("https://api.spotify.com");
    client.set_address_family(AF_INET);
    // std::string url = "/v1/me/top/artists?time_range=" + timeRange + "&limit=" + std::to_string(limit) +
    //                   "&offset=" + std::to_string(offset);
    std::string url = "/v1/me/top/artists";
    std::string authHeader = "Bearer " + *accessToken;
    httplib::Headers headers = { { "Authorization", authHeader } };

    auto res = client.Get(url, headers);

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
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
    } else {
        std::cerr << "HTTP request failed, error code: " << static_cast<int>(res.error()) << std::endl;
        return {};
    }
}

/*
 * Get the current user’s top tracks based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
UserTopTracksData SpotifyWebAPI::getUserTopTracks(const std::string &timeRange, int limit, int offset) {
    httplib::Client client("https://api.spotify.com");
    client.set_address_family(AF_INET);
    std::string url = "/v1/me/top/tracks?time_range=" + timeRange + "&limit=" + std::to_string(limit) +
                      "&offset=" + std::to_string(offset);

    std::string authHeader = "Bearer " + *accessToken;
    httplib::Headers headers = { { "Authorization", authHeader } };

    auto res = client.Get(url, headers);

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
        UserTopTracksData data;
        for (const auto &album : response["items"]) {
            data.tracks.push_back(album["name"]);
        }
        return data;
    } else {
        std::cerr << "HTTP request failed, error code: " << static_cast<int>(res.error()) << std::endl;
        return {};
    }
}
