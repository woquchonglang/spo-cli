module;
export module event;

import std;
import spotifyAuth;
import spotifyWebAPI;
import concurrentqueue;

export namespace SPOCLI {

enum class Event {
    Exit,
    Login,

    GetDevices,
    GetBrowseCategories,

    GetUserProfile,
    GetUserTopArtists,
    GetUserTopTracks,
    GetUserFollowedPlaylist,
    GetUserFollowedArtists,
    GetUserfollowArtistOrUsers,

    GetUserPlaylists,
    GetUserSavedAlbums,
    GetUserSavedShows,
    GetUserSavedTracks,
};

}

export class EventHandler {
public:
    EventHandler(moodycamel::ConcurrentQueue<SPOCLI::Event> &queue, SpotifyAuth &auth)
            : eventQueue(queue), api(nullptr), auth(auth) {};

    void handle(std::stop_token st, std::shared_ptr<SpotifyData> spotifyData);

    void scheduleExitHandler(std::function<void()> handler) { exitHandler.push_back(handler); }

private:
    moodycamel::ConcurrentQueue<SPOCLI::Event> &eventQueue;

    SpotifyWebAPI api;
    SpotifyAuth &auth;

    std::vector<std::function<void()>> exitHandler;
};
