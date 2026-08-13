module;
#include <sys/socket.h>
export module spotifyWebAPI;

import httplib;
import nlohmann.json;
import concurrentqueue;
import std;
import rwlock;

export struct Image {
    std::string url;
    int height = 0;
    int width = 0;
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
    RwLock<UserProfile> userProfile;
    RwLock<UserTopArtistsData> topArtists;
    RwLock<UserTopTracksData> topTracks;

    RwLock<bool> isLogined;

    SpotifyData() : userProfile({}), topArtists({}), topTracks({}), isLogined(false) {}
};

export class SpotifyWebAPI {
public:
    SpotifyWebAPI(std::shared_ptr<std::string> accessToken);

    UserProfile getUserProfile();
    UserTopArtistsData getUserTopArtists(const std::string &timeRange = "medium_term", int limit = 20, int offset = 0);
    UserTopTracksData getUserTopTracks(const std::string &timeRange = "medium_term", int limit = 20, int offset = 0);
    void getUserFollowPlaylist();
    void userUnfollowPlaylist();
    void getUserFollowedArtists();
    void getUserfollowArtistOrUsers();
    void userUnfollowArtistOrUsers();
    void checkIfUserFollowsArtistsOrUsers();
    void checkIfCurrentUserFollowsPlaylist();

    UserProfile async_getUserProfile();

private:
    std::shared_ptr<std::string> accessToken;
};
