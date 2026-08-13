module;
export module spotifyAuth;

import config;
import std;
import timer;

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

export class SpotifyAuth {
public:
    SpotifyAuth(const Config &config);
    bool login();
    std::shared_ptr<std::string> getAccessToken() { return access_token; }

private:
    void exchangeCodeForToken(std::string_view code);
    void refreshAccessToken();

private:
    const Config &config;
    std::shared_ptr<std::string> access_token = std::make_shared<std::string>();
    std::string refresh_token;
    int expires_in;
};
