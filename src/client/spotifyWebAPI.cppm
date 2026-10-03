module;
#include <sys/socket.h>
export module spotifyWebAPI;

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

    void getUserProfile(std::function<void()> cb);
    void getUserTopArtists(std::function<void()> cb,
                           const std::string &timeRange = "medium_term",
                           int limit = 20, int offset = 0);
    void getUserTopTracks(std::function<void()> cb,
                          const std::string &timeRange = "medium_term",
                          int limit = 20, int offset = 0);

    // player
    void TransferPlayback(std::function<void()> cb, bool play = true);
    void GetCurrentlyPlayingTrack(std::function<void()> cb);
    void GetAvailableDevices(std::function<void()> cb);
    void GetUserQueue(std::function<void()> cb);
    void ResumePlayback(std::function<void()> cb);
    void StartPlayback(std::function<void()> cb, const PlayTarget play_target,
                       const std::string &device_id = "", const int offset = 0,
                       const int position_ms = 0);
    void PausePlayback(std::function<void()> cb);

    void GetUserPlaylists(std::function<void()> cb, int limit = 20,
                          int offset = 0);

    void skipToNext(std::function<void()> cb);
    void skipToPrevious(std::function<void()> cb);
    void SeekPosition(std::function<void()> cb, const int position_ms);
    void setPlaybackVolume(const int volume, std::string device_id = "");

    // tracks
    void getTrack(std::string_view id);

public:
    SpotifyData &spotifyData;

private:
    std::shared_ptr<std::string> accessToken;
    std::unique_ptr<APIImpl> apiImpl;
};
