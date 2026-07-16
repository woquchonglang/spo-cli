module;
export module event;

import concurrentqueue;
import ftxui;
import std;
import spotifyAuth;
import spotifyWebAPI;

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
            : eventQueue(queue),api(nullptr),auth(auth){};

    void handle(std::stop_token st, std::shared_ptr<SpotifyData> spotifyData);

    void scheduleExitHandler(std::function<void()> handler) { exitHandler.push_back(handler); }

private:
    moodycamel::ConcurrentQueue<SPOCLI::Event> &eventQueue;

    SpotifyWebAPI api;
    SpotifyAuth &auth;

    std::vector<std::function<void()>> exitHandler;
};

void EventHandler::handle(std::stop_token st, std::shared_ptr<SpotifyData> spotifyData) {
    while (!st.stop_requested()) {
        SPOCLI::Event event;
        if (eventQueue.try_dequeue(event)) {
            switch (event) {
            case SPOCLI::Event::Login:
                spotifyData->isLogined.write().value = auth.login();
                api = SpotifyWebAPI(auth.getAccessToken());
                break;
            case SPOCLI::Event::GetUserProfile:
                spotifyData->userProfile.write().value = api.getUserProfile();
                ftxui::animation::RequestAnimationFrame();
                break;
            case SPOCLI::Event::GetUserTopArtists:
                spotifyData->topArtists.write().value = api.getUserTopArtists();
                ftxui::animation::RequestAnimationFrame();
                break;
            case SPOCLI::Event::GetUserTopTracks:
                spotifyData->topTracks.write().value = api.getUserTopTracks();
                ftxui::animation::RequestAnimationFrame();
                break;
            case SPOCLI::Event::Exit:
                for (auto &handler : exitHandler) {
                    handler();
                }
                break;
            default:
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
