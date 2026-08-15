module;
module event;

import concurrentqueue;
import ftxui;
import std;
import spotifyAuth;
import spotifyWebAPI;

void EventHandler::handle(std::stop_token st) {
    while (!st.stop_requested()) {
        SPOCLI::Event event;
        if (eventQueue.try_dequeue(event)) {
            switch (event) {
            case SPOCLI::Event::Login:

                if (auth.login())
                    api.updateAccessToken(auth.getAccessToken());
                break;
            case SPOCLI::Event::GetUserProfile:
                api.getUserProfile(ftxui::animation::RequestAnimationFrame);
                break;
            case SPOCLI::Event::GetUserTopArtists:
                api.getUserTopArtists(ftxui::animation::RequestAnimationFrame);
                break;
            case SPOCLI::Event::GetUserTopTracks:
                api.getUserTopTracks(ftxui::animation::RequestAnimationFrame);
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
