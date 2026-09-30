module;
#include <sys/socket.h>
export module spotifyWebAPI;

import httplib;
import nlohmann.json;
import spsc;
import std;
import rwlock;
export import spotify_data;


struct APIImpl;

export class SpotifyWebAPI {
public:
    SpotifyWebAPI(std::shared_ptr<std::string> accessToken, SpotifyData &data);
    ~SpotifyWebAPI();

    void updateAccessToken(std::shared_ptr<std::string> token);


    void getUserFollowPlaylist();
    void userUnfollowPlaylist();
    void getUserFollowedArtists();
    void getUserfollowArtistOrUsers();
    void userUnfollowArtistOrUsers();
    void checkIfUserFollowsArtistsOrUsers();
    void checkIfCurrentUserFollowsPlaylist();

    void getUserProfile(void (*cb)());
    void getUserTopArtists(void (*cb)(),
                           const std::string &timeRange = "medium_term",
                           int limit = 20, int offset = 0);
    void getUserTopTracks(void (*cb)(),
                          const std::string &timeRange = "medium_term",
                          int limit = 20, int offset = 0);

    // player
    void GetCurrentlyPlayingTrack(void (*cb)());
    void GetUserQueue(void (*cb)());
    void ResumePlayback(void (*cb)());
    void StartPlayback(void (*cb)(), const PlayTarget play_target,
                       const std::string &device_id = "", const int offset = 0,
                       const int position_ms = 0);
    void PausePlayback(void (*cb)());

    void GetUserPlaylists(void (*cb)(), int limit = 20, int offset = 0);

    void SeekPosition(void (*cb)(), const int position_ms);

    // tracks
    void getTrack(std::string_view id);

    // librespot
    void playPause(void (*cb)());

public:
    SpotifyData &spotifyData;

private:
    std::shared_ptr<std::string> accessToken;
    std::unique_ptr<APIImpl> apiImpl;
};
