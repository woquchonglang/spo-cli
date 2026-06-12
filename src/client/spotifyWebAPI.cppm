module;
#include <sys/socket.h>
export module spotifyWebAPI;

import httplib;
import nlohmann.json;
import concurrentqueue;
import std;
import rwlock;

struct Image {
    std::string url;
    int height;
    int width;
};

struct SizesImages {
    std::vector<Image> largeImage;
    std::vector<Image> mediumImage;
    std::vector<Image> smallImage;
};

struct UserProfile {
    std::string account_id;
    std::string country;
    std::string displayName;
    std::string email;
    std::string product;
    Image image;
};

struct UserTopArtistsData {
    std::vector<std::string> names;
    std::vector<std::string> uri;
    SizesImages image;
};

struct UserTopTracksData {
    std::vector<std::string> uri;
    std::vector<std::string> tracks;
    SizesImages image;
};

export struct SpotifyData {
    RwLock<std::optional<UserProfile>> userProfile;
    RwLock<std::optional<UserTopArtistsData>> topArtists;
    RwLock<std::optional<UserTopTracksData>> topTracks;

    SpotifyData() : userProfile(std::nullopt), topArtists(std::nullopt), topTracks(std::nullopt) {}
};

export class SpotifyWebAPI {
public:
    SpotifyWebAPI(std::shared_ptr<std::string> accessToken);

    std::optional<UserProfile> getUserProfile();
    std::optional<UserTopArtistsData> getUserTopArtists(const std::string &timeRange = "medium_term", int limit = 20,
                                                        int offset = 0);
    std::optional<UserTopTracksData> getUserTopTracks(const std::string &timeRange = "medium_term", int limit = 20,
                                                      int offset = 0);
    void getUserFollowPlaylist();
    void userUnfollowPlaylist();
    void getUserFollowedArtists();
    void getUserfollowArtistOrUsers();
    void userUnfollowArtistOrUsers();
    void checkIfUserFollowsArtistsOrUsers();
    void checkIfCurrentUserFollowsPlaylist();

private:
    std::shared_ptr<std::string> accessToken;
};

SpotifyWebAPI::SpotifyWebAPI(std::shared_ptr<std::string> accessToken) : accessToken(accessToken) {};

std::optional<UserProfile> SpotifyWebAPI::getUserProfile() {
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
        return std::nullopt;
    }
}

/*
 * Get the current user’s top artists based on calculated affinity.
 * time_range: Valid values: long_term (calculated from ~1 year of data and including all new data as it becomes available),
 *             medium_term (approximately last 6 months),
 *             short_term (approximately last 4 weeks).
 *             Default: medium_term
 *
 */
std::optional<UserTopArtistsData> SpotifyWebAPI::getUserTopArtists(const std::string &timeRange, int limit,
                                                                   int offset) {
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
        return std::nullopt;
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
std::optional<UserTopTracksData> SpotifyWebAPI::getUserTopTracks(const std::string &timeRange, int limit, int offset) {
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
        return std::nullopt;
    }
}
