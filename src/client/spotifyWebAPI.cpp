module;
#include <sys/socket.h>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
module spotifyWebAPI;

import http.client;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

struct APIImpl {
    APIImpl()
            : work(boost::asio::make_work_guard(ioc))
            , ctx(ssl::context::tls_client) {
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);
        asio_thread = std::thread([this]() { ioc.run(); });
    }
    ~APIImpl() {
        work.reset();
        ioc.stop();
        if (asio_thread.joinable()) {
            asio_thread.join();
        }
    }

    template <typename T>
    net::awaitable<T> async_http_get(const std::string &host,
                                     const std::string &target,
                                     const std::string &token);

    net::awaitable<void> async_http_put(const std::string &host,
                                        const std::string &target,
                                        const std::string &token);

    net::awaitable<void> async_http_put(const std::string &host,
                                        const std::string &target,
                                        const std::string &token,
                                        const std::string &body);

    net::awaitable<void> async_http_post(const std::string &host,
                                         const std::string &target,
                                         const std::string &token);

    template <typename T>
    void fetchData(const std::string &endpoint, const std::string &target,
                   const std::string &accessToken,
                   std::function<void(T)> onComplete);

    void fetch(const std::string &endpoint, const std::string &target,
               const std::string &accessToken,
               std::function<void()> onComplete);

    void put_data(const std::string &endpoint, const std::string &target,
                  const std::string &accessToken, std::string body,
                  std::function<void()> onComplete);

    void post(const std::string &endpoint, const std::string &target,
              const std::string &accessToken, std::function<void()> onComplete);

    boost::asio::io_context ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
            work;
    std::thread asio_thread;

    ssl::context ctx;
    static constexpr std::string_view host = "api.spotify.com";
};

template <typename T> T parse_response(const nlohmann::json &response) {
    return T{};
}

template <typename T>
net::awaitable<T> APIImpl::async_http_get(const std::string &host,
                                          const std::string &target,
                                          const std::string &token) {
    http::request<http::string_body> req{ http::verb::get, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::user_agent, "Beast/1.0");

    auto res = co_await client_read(host, "443", ctx, std::move(req));

    if (res.result_int() == 200) {
        nlohmann::json response = nlohmann::json::parse(
                beast::buffers_to_string(res.body().data()));
        auto data = parse_response<T>(response);
        co_return data;
    } else {
        co_return T{};
    }
}

net::awaitable<void> APIImpl::async_http_put(const std::string &host,
                                             const std::string &target,
                                             const std::string &token) {
    http::request<http::string_body> req{ http::verb::put, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::user_agent, "Beast/1.0");
    req.prepare_payload();

    auto res = co_await client_read(host, "443", ctx, std::move(req));

    auto result = res.result_int();
    if (result == 200 || result == 204) {
        co_return;
    }
}

net::awaitable<void> APIImpl::async_http_put(const std::string &host,
                                             const std::string &target,
                                             const std::string &token,
                                             const std::string &body) {
    http::request<http::string_body> req{ http::verb::put, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::user_agent, "Beast/1.0");
    req.body() = std::move(body);
    req.prepare_payload();

    auto res = co_await client_read(host, "443", ctx, std::move(req));

    auto result = res.result_int();
    if (result != 204) {
        printf("put data error\n");
    }
    co_return;
}

net::awaitable<void> APIImpl::async_http_post(const std::string &host,
                                              const std::string &target,
                                              const std::string &token) {
    http::request<http::string_body> req{ http::verb::post, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::user_agent, "Beast/1.0");
    req.prepare_payload();

    auto res = co_await client_read(host, "443", ctx, std::move(req));

    auto result = res.result_int();
    if (result != 203) {
        printf("http post error");
    }
    co_return;
}

template <typename T>
void APIImpl::fetchData(const std::string &endpoint, const std::string &target,
                        const std::string &accessToken,
                        std::function<void(T)> onComplete) {
    net::co_spawn(
            ioc,
            [this, endpoint, target, accessToken,
             onComplete]() -> boost::asio::awaitable<void> {
                try {
                    auto data = co_await async_http_get<T>(endpoint, target,
                                                           accessToken);
                    if (onComplete)
                        onComplete(data);

                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                    if (onComplete)
                        onComplete(T{});
                }
            },
            boost::asio::detached);
}

void APIImpl::fetch(const std::string &endpoint, const std::string &target,
                    const std::string &accessToken,
                    std::function<void()> onComplete) {
    net::co_spawn(
            ioc,
            [this, endpoint, target, accessToken,
             onComplete]() -> boost::asio::awaitable<void> {
                try {
                    co_await async_http_put(endpoint, target, accessToken);
                    if (onComplete)
                        onComplete();
                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                    if (onComplete)
                        onComplete();
                }
            },
            boost::asio::detached);
}

void APIImpl::put_data(const std::string &endpoint, const std::string &target,
                       const std::string &accessToken, std::string body,
                       std::function<void()> onComplete) {
    net::co_spawn(
            ioc,
            [this, endpoint, target, accessToken, body,
             onComplete]() -> boost::asio::awaitable<void> {
                try {
                    co_await async_http_put(endpoint, target, accessToken,
                                            body);
                    if (onComplete)
                        onComplete();
                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                    if (onComplete)
                        onComplete();
                }
            },
            boost::asio::detached);
}

void APIImpl::post(const std::string &endpoint, const std::string &target,
                   const std::string &accessToken,
                   std::function<void()> onComplete) {
    net::co_spawn(
            ioc,
            [this, endpoint, target, accessToken,
             onComplete]() -> boost::asio::awaitable<void> {
                try {
                    co_await async_http_post(endpoint, target, accessToken);
                    if (onComplete)
                        onComplete();
                } catch (const std::exception &e) {
                    std::cerr << "Async error: " << e.what() << std::endl;
                    if (onComplete)
                        onComplete();
                }
            },
            boost::asio::detached);
}


SpotifyWebAPI::SpotifyWebAPI(std::shared_ptr<std::string> accessToken,
                             SpotifyData &data)
        : accessToken(accessToken)
        , spotifyData(data)
        , apiImpl(std::make_unique<APIImpl>()) {};

SpotifyWebAPI::~SpotifyWebAPI() {}

void SpotifyWebAPI::updateAccessToken(std::shared_ptr<std::string> token) {
    spotifyData.isLogined.write().value = true;
    accessToken = token;
}

void SpotifyWebAPI::getUserProfile(std::function<void()> cb) {
    apiImpl->fetchData<UserProfile>(
            "api.spotify.com", "/v1/me", *accessToken.get(),
            [this, cb](auto profile) {
                spotifyData.userProfile.write().value = profile;
                if (cb) {
                    cb();
                }
            });
}

/*
 * Get the current user’s top artists based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
void SpotifyWebAPI::getUserTopArtists(std::function<void()> cb,
                                      const std::string &timeRange, int limit,
                                      int offset) {
    std::string url = "/v1/me/top/artists";
    apiImpl->fetchData<UserTopArtistsData>(
            "api.spotify.com", url, *accessToken.get(),
            [this, cb](auto artists) {
                spotifyData.topArtists.write().value = artists;
                if (cb) {
                    cb();
                }
            });
}

/*
 * Get the current user’s top tracks based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
void SpotifyWebAPI::getUserTopTracks(std::function<void()> cb,
                                     const std::string &timeRange, int limit,
                                     int offset) {
    std::string url = "/v1/me/top/tracks?time_range=" + timeRange +
                      "&limit=" + std::to_string(limit) +
                      "&offset=" + std::to_string(offset);
    apiImpl->fetchData<UserTopTracksData>(
            "api.spotify.com", url, *accessToken.get(),
            [this, cb](auto tracks) {
                spotifyData.topTracks.write().value = tracks;
                if (cb) {
                    cb();
                }
            });
}

void SpotifyWebAPI::GetUserPlaylists(std::function<void()> cb, int limit,
                                     int offset) {
    std::string url = "/v1/me/playlists?limit=" + std::to_string(limit) +
                      "&offset=" + std::to_string(offset);

    apiImpl->fetchData<UserPlaylistsData>(
            "api.spotify.com", url, *accessToken.get(),
            [this, cb](auto playlists) {
                spotifyData.userPlaylists.write().value = playlists;
                if (cb) {
                    cb();
                }
            });
}

// player

void SpotifyWebAPI::TransferPlayback(std::function<void()> cb, bool play) {
    std::string url = "/v1/me/player";
    auto guard = spotifyData.devices.read();
    if (!guard.value.empty()) {
        std::string body = std::format(R"({{"device_ids":["{}"],"play":{}}})",
                                       guard.value[0].id,
                                       play ? "true" : "false");
        apiImpl->put_data("api.spotify.com", url, *accessToken.get(), body, cb);
    }
}

void SpotifyWebAPI::GetCurrentlyPlayingTrack(std::function<void()> cb) {
    std::string url = "/v1/me/player/currently-playing";

    apiImpl->fetchData<CurrentlyPlayingTrack>(
            "api.spotify.com", url, *accessToken.get(), [this, cb](auto track) {
                spotifyData.currentlyPlayTrack.write().value = track;
                spotifyData.update_playback_last_updated_time();
                if (cb) {
                    cb();
                }
            });
}

void SpotifyWebAPI::GetAvailableDevices(std::function<void()> cb) {
    std::string url = "/v1/me/player/devices";
    apiImpl->fetchData<std::vector<Device>>(
            "api.spotify.com", url, *accessToken.get(),
            [cb, this](auto devices) {
                spotifyData.devices.write().value = devices;
                cb();
            });
}

void SpotifyWebAPI::GetUserQueue(std::function<void()> cb) {
    std::string url = "/v1/me/player/queue";

    apiImpl->fetchData<UserQueueData>(
            "api.spotify.com", url, *accessToken.get(),
            [this, cb](auto userQueue) {
                spotifyData.userQueue.write().value = userQueue;
                if (cb) {
                    cb();
                }
            });
}

void SpotifyWebAPI::StartPlayback(std::function<void()> cb,
                                  const PlayTarget play_target,
                                  const std::string &device_id,
                                  const int offset, const int position_ms) {
    std::string url = "/v1/me/player/play";

    std::visit(
            overloaded{
                    [&url](std::monostate) {},
                    [this, &cb, &url, &offset,
                     &position_ms](const std::string &uri) {
                        std::string body;
                        if (uri.starts_with("spotify:track:")) {
                            body = std::format(
                                    R"({{"uris":["{}"],"position_ms":{}}})",
                                    uri, position_ms);
                        } else {
                            // TODO: add offset
                            body = std::format(
                                    R"({{"context_uri":"{}","position_ms":{}}})",
                                    uri, position_ms);
                        }
                        auto cbImpl = [cb, this]() {
                            spotifyData.currentlyPlayTrack.write()
                                    .value.isPlaying = true;
                            cb();
                            this->GetCurrentlyPlayingTrack(nullptr);
                        };
                        apiImpl->put_data("api.spotify.com", url,
                                          *accessToken.get(), body, cbImpl);
                    },
                    [&url](std::vector<std::string> uris) {},
            },
            play_target);
}

void SpotifyWebAPI::ResumePlayback(std::function<void()> cb) {
    std::string url = "/v1/me/player/play";
    auto cbImpl = [cb, this]() {
        spotifyData.currentlyPlayTrack.write().value.isPlaying = true;
        cb();
    };
    apiImpl->fetch("api.spotify.com", url, *accessToken.get(), cbImpl);
}

void SpotifyWebAPI::PausePlayback(std::function<void()> cb) {
    std::string url = "/v1/me/player/pause";
    auto cbImpl = [cb, this]() {
        this->spotifyData.currentlyPlayTrack.write().value.isPlaying = false;
        cb();
    };
    apiImpl->fetch("api.spotify.com", url, *accessToken.get(), cbImpl);
}

void SpotifyWebAPI::skipToNext(std::function<void()> cb) {
    std::string url = "/v1/me/player/next";
    auto cbImpl = [cb, this]() { cb(); };
    apiImpl->post("api.spotify.com", url, *accessToken.get(), cbImpl);
}

void SpotifyWebAPI::skipToPrevious(std::function<void()> cb) {
    std::string url = "/v1/me/player/previous";
    auto cbImpl = [cb, this]() { cb(); };
    apiImpl->post("api.spotify.com", url, *accessToken.get(), cbImpl);
}

void SpotifyWebAPI::SeekPosition(std::function<void()> cb,
                                 const int position_ms) {
    std::string url =
            "/v1/me/player/seek?position_ms=" + std::to_string(position_ms);
    auto cbImpl = [cb, this]() {
        this->spotifyData.currentlyPlayTrack.write().value.isPlaying = false;
        cb();
    };
    apiImpl->fetch("api.spotify.com", url, *accessToken.get(), cbImpl);
}

void SpotifyWebAPI::setPlaybackVolume(const int volume, std::string device_id) {
    std::string url =
            "/v1/me/player/volume?volume_percent=" + std::to_string(volume);
    if (!device_id.empty()) {
        url += "&device_id=" + device_id;
    }
    auto cbImpl = [this, volume]() {
        if (this->spotifyData.devices.write().value.size() > 0) {
            this->spotifyData.devices.write().value[0].volume_percent = volume;
        }
    };
    apiImpl->fetch("api.spotify.com", url, *accessToken.get(), cbImpl);
}

template <>
UserProfile parse_response<UserProfile>(const nlohmann::json &response) {
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

template <>
UserTopArtistsData
parse_response<UserTopArtistsData>(const nlohmann::json &response) {
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

template <>
UserTopTracksData
parse_response<UserTopTracksData>(const nlohmann::json &response) {
    UserTopTracksData data;
    for (const auto &album : response["items"]) {
        data.tracks.push_back(album["name"]);
        data.uri.push_back(album["uri"]);
    }
    return data;
}

template <>
UserPlaylistsData
parse_response<UserPlaylistsData>(const nlohmann::json &response) {
    UserPlaylistsData data;
    for (const auto &item : response["items"]) {
        data.items.push_back(item["name"]);
    }
    return data;
}

template <>
CurrentlyPlayingTrack
parse_response<CurrentlyPlayingTrack>(const nlohmann::json &response) {
    CurrentlyPlayingTrack data{};
    data.progress_ms = response["progress_ms"];
    data.isPlaying = response["is_playing"];
    if (response.contains("device")) {
        auto device = response["device"];
        data.device.id = device["id"];
        data.device.is_active = device["is_active"];
        data.device.is_private_session = device["is_private_session"];
        data.device.is_restricted = device["is_restricted"];
        data.device.name = device["name"];
        data.device.type = device["type"];
        data.device.volume_percent = device["volume_percent"];
        data.device.supports_volume = device["supports_volume"];
    }
    auto item = response["item"];
    if (!item.is_null()) {
        data.currently_playing.name = item["name"];
        data.currently_playing.id = item["id"];
        data.currently_playing.uri = item["uri"];
        for (const auto &artist : item["artists"]) {
            data.currently_playing.atrists_name.push_back(artist["name"]);
        }
        data.currently_playing.duration_ms = item["duration_ms"];
        auto &images = item["images"];
        if (images.is_array() && !images.empty() && !images[0].is_null()) {
            Image img;
            img.url = images[0]["url"];
            img.height = images[0]["height"];
            img.width = images[0]["width"];
            data.currently_playing.image.largeImage.push_back(img);
        }
        if (images.is_array() && !images.empty() && !images[1].is_null()) {
            Image img;
            img.url = images[1]["url"];
            img.height = images[1]["height"];
            img.width = images[1]["width"];
            data.currently_playing.image.mediumImage.push_back(img);
        }
        if (images.is_array() && !images.empty() && !images[2].is_null()) {
            Image img;
            img.url = images[2]["url"];
            img.height = images[2]["height"];
            img.width = images[2]["width"];
            data.currently_playing.image.smallImage.push_back(img);
        }
    }
    return data;
}

template <>
std::vector<Device>
parse_response<std::vector<Device>>(const nlohmann::json &response) {
    auto datas = response["devices"];
    std::vector<Device> devices;
    if (!datas.is_null()) {
        for (auto &data : datas) {
            Device device;
            device.id = data.value("id", "");
            device.is_active = data.value("is_active", false);
            device.is_private_session = data.value("is_private_session", false);
            device.is_restricted = data.value("is_restricted", false);
            device.name = data.value("name", "");
            device.type = data.value("type", "");
            device.volume_percent = data.value("volume_percent", 0);
            device.supports_volume = data.value("supports_volume", false);
            devices.push_back(std::move(device));
        }
    }
    return devices;
}

template <>
UserQueueData parse_response<UserQueueData>(const nlohmann::json &response) {
    UserQueueData data{};
    auto currently_playing = response["currently_playing"];
    if (!currently_playing.is_null()) {
        data.currently_playing.name = currently_playing["name"];
        data.currently_playing.id = currently_playing["id"];
        data.currently_playing.uri = currently_playing["uri"];
        for (const auto &artist : currently_playing["artists"]) {
            data.currently_playing.atrists_name.push_back(artist["name"]);
        }
        data.currently_playing.duration_ms = currently_playing["duration_ms"];
    }
    return data;
}
