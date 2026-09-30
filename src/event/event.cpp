module;
module event;

import ftxui;
import std;
import spotifyAuth;
import spotifyWebAPI;
import mpris;

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
                api.getUserProfile([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });

                break;
            case SPOCLI::Event::GetUserTopArtists:
                api.getUserTopArtists([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });

                break;
            case SPOCLI::Event::GetUserTopTracks:
                api.getUserTopTracks([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });

                break;
            case SPOCLI::Event::GetUserPlaylists:
                api.GetUserPlaylists([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });

                break;
                // player
            case SPOCLI::Event::GetCurrentlyPlayingTrack:
                api.GetCurrentlyPlayingTrack([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });
                break;
            case SPOCLI::Event::GetUserQueue:
                api.GetUserQueue([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });

                break;
            case SPOCLI::Event::SeekRight: {
                auto guard1 = api.spotifyData.currentlyPlayTrack.read();
                api.SeekPosition(
                        []() { ftxui::animation::RequestAnimationFrame(); },
                        guard1.value.progress_ms + 300);
            } break;
            case SPOCLI::Event::SeekLeft: {
                auto guard1 = api.spotifyData.currentlyPlayTrack.read();
                api.SeekPosition(
                        []() { ftxui::animation::RequestAnimationFrame(); },
                        guard1.value.progress_ms - 300);
            } break;
            case SPOCLI::Event::Resume:
                api.ResumePlayback([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });
                break;
            case SPOCLI::Event::Pause:
                api.PausePlayback([]() {
                    ftxui::animation::RequestAnimationFrame();
                    mpris_notify_spsc.enqueue(MpricEvent::all_update);
                });
                break;
            case SPOCLI::Event::Exit:
                for (auto &handler : exitHandler) {
                    handler();
                }
                break;
            case SPOCLI::Event::Refresh:
                ftxui::animation::RequestAnimationFrame();
                break;
            case SPOCLI::Event::GetLyrics:
                librespot.update_lyrics();
                break;
            default:
                break;
            }
        }

        std::string uri;
        if (play_song_spsc.try_dequeue(uri)) {
            api.StartPlayback(ftxui::animation::RequestAnimationFrame, uri);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
