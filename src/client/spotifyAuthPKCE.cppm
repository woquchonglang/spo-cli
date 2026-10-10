module;
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
export module spotifyAuthPKCE;

import config;
import std;
import nlohmann.json;

std::vector<std::string> scopes = {
    // Images
    "ugc-image-upload",
    // Spotify Connect
    "user-read-playback-state",
    "user-modify-playback-state",
    "user-read-currently-playing",
    // Playback
    "app-remote-control",
    "streaming",
    // Playlists
    "playlist-read-private",
    "playlist-read-collaborative",
    "playlist-modify-private",
    "playlist-modify-public",
    // Follow
    "user-follow-modify",
    "user-follow-read",
    // Listening History
    "user-read-playback-position",
    "user-top-read",
    "user-read-recently-played",
    // Library
    "user-library-modify",
    "user-library-read",
    // Users
    "user-read-email",
    "user-read-private",
};

struct TokenCache {
    std::string access_token;
    std::string refresh_token;
    int expires_in;
    std::chrono::system_clock::time_point token_acquire_time;
};


export class SpotifyAuthPKCE {
public:
    SpotifyAuthPKCE(boost::asio::io_context &ioc, const Config &config);
    ~SpotifyAuthPKCE();
    boost::asio::awaitable<bool> login();
    std::shared_ptr<std::string> getAccessToken() { return access_token; }

private:
    void exchangeCodeForToken(std::string_view code);
    void saveTokenCache(const TokenCache &cache,
                        const std::filesystem::path &cache_path);
    std::optional<TokenCache>
    loadTokenCache(const std::filesystem::path &cachePath);
    boost::asio::awaitable<nlohmann::json>
    async_http_post(const std::string &host, const std::string &target,
                    const std::string &body);
    boost::asio::awaitable<void> startRefreshTimer();
    boost::asio::awaitable<void> refreshAccessToken();
    boost::asio::awaitable<nlohmann::json> async_http_refreshtoken_post();

private:
    const Config &config;
    std::string codeVerifier;
    std::shared_ptr<std::string> access_token = std::make_shared<std::string>();
    std::string refresh_token;
    int expires_in;
    std::filesystem::path cache_file;
    boost::asio::io_context &ioc;
    boost::asio::ssl::context ctx;
    std::optional<TokenCache> cache;
};
