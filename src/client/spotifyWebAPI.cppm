module;
// #include <sys/socket.h>
export module spotify:WebAPI;

import nlohmann.json;
import spsc;
import std;
import rwlock;
import asio;
import spotify.data;

struct APIImpl;

export class SpotifyWebAPI {
public:
    SpotifyWebAPI(asio::io_context &ioc,
                  std::shared_ptr<std::string> accessToken, SpotifyData &data);
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
    asio::awaitable<void> TransferPlayback(bool play = true);
    void GetCurrentlyPlayingTrack(std::function<void()> cb);
    asio::awaitable<void> GetCurrentlyPlayingTrack();
    void GetAvailableDevices(std::function<void()> cb);
    asio::awaitable<void> AsyncGetAvailableDevices();
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
    asio::awaitable<void> AsyncSetPlaybackVolume(const int volume,
                                                 std::string device_id = "");


    // playlists
    asio::awaitable<void> GetUserPlaylistItems(
            const std::string &playlist_id, const std::string &market = "",
            const std::string &fields = "", const int limit = 20,
            const int offset = 0, const std::string &additional_types = "");

    // search
    asio::awaitable<void> Search(const std::string &q,
                                 const std::string &type = "",
                                 const std::string &market = "",
                                 const int limit = 10, const int offset = 0,
                                 const std::string &include_external = "");


    // tracks
    void getTrack(std::string_view id);

public:
    SpotifyData &spotifyData;

private:
    std::shared_ptr<std::string> accessToken;
    std::unique_ptr<APIImpl> apiImpl;
    std::string endpoint = "api.spotify.com";
};
