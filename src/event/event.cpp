module;
 module event;

import concurrentqueue;
import ftxui;
import std;
import spotifyAuth;
import spotifyWebAPI;

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
