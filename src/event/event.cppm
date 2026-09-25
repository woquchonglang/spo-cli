module;
export module event;

import std;
import spotifyAuth;
import spotifyWebAPI;
import spsc;

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

export moodycamel::Spsc<SPOCLI::Event> eventQueue;

export class EventHandler {
public:
    EventHandler(moodycamel::Spsc<SPOCLI::Event> &queue,
                 SpotifyAuth &auth, std::shared_ptr<SpotifyData> spotifyData)
            : eventQueue(queue), api(nullptr, spotifyData), auth(auth) {};

    void handle(std::stop_token st);

    void scheduleExitHandler(std::function<void()> handler) {
        exitHandler.push_back(handler);
    }

private:
    moodycamel::Spsc<SPOCLI::Event> &eventQueue;

    SpotifyWebAPI api;
    SpotifyAuth auth;

    std::vector<std::function<void()>> exitHandler;
};
