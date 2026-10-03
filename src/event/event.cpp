module;
#include <boost/asio.hpp>
module event;

import ftxui;
import std;
import spotifyWebAPI;

AsyncEventHandler::AsyncEventHandler(boost::asio::io_context &ioc,
                                     Config &config, SpotifyData &spotifyData,
                                     Librespot &librespot)
        : api(nullptr, spotifyData)
        , auth(ioc, config)
        , librespot(librespot)
        , ioc(ioc) {};

boost::asio::awaitable<void> AsyncEventHandler::handle() {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    while (true) {
        SPOCLI::Event event;
        if (eventQueue.try_dequeue(event)) {
            switch (event) {
            case SPOCLI::Event::Login: {
                boost::asio::co_spawn(
                        ioc,
                        [this]() -> boost::asio::awaitable<void> {
                            if (co_await auth.login()) {
                                api.updateAccessToken(auth.getAccessToken());
                                boost::asio::steady_timer timer(
                                        co_await boost::asio::this_coro::
                                                executor);
                                timer.expires_after(std::chrono::seconds(10));
                                auto [ec] = co_await timer.async_wait(
                                        boost::asio::as_tuple(
                                                boost::asio::use_awaitable));
                                if (ec)
                                    co_return;

                                api.GetAvailableDevices([this]() {
                                    api.TransferPlayback([this]() {
                                        this->api.GetCurrentlyPlayingTrack(
                                                nullptr);
                                    });
                                });
                            }
                        },
                        boost::asio::detached);
            } break;
            case SPOCLI::Event::GetUserProfile:
                api.getUserProfile(
                        []() { ftxui::animation::RequestAnimationFrame(); });

                break;
            case SPOCLI::Event::GetUserTopArtists:
                api.getUserTopArtists(
                        []() { ftxui::animation::RequestAnimationFrame(); });

                break;
            case SPOCLI::Event::GetUserTopTracks:
                api.getUserTopTracks(
                        []() { ftxui::animation::RequestAnimationFrame(); });
                break;
            case SPOCLI::Event::GetUserFollowedPlaylist:
                break;
            case SPOCLI::Event::GetUserPlaylists:
                api.GetUserPlaylists(
                        []() { ftxui::animation::RequestAnimationFrame(); });

                break;
                // player
            case SPOCLI::Event::GetCurrentlyPlayingTrack:
                api.GetCurrentlyPlayingTrack(
                        []() { ftxui::animation::RequestAnimationFrame(); });
                break;
            case SPOCLI::Event::GetAvailableDevice:
                api.GetAvailableDevices(nullptr);
                break;
            case SPOCLI::Event::GetUserQueue:
                api.GetUserQueue(
                        []() { ftxui::animation::RequestAnimationFrame(); });
                break;
            case SPOCLI::Event::SkipToNext: {
                api.skipToNext(
                        []() { ftxui::animation::RequestAnimationFrame(); });
            } break;
            case SPOCLI::Event::SkipToPrevious: {
                api.skipToPrevious(
                        []() { ftxui::animation::RequestAnimationFrame(); });
            } break;
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
                api.ResumePlayback(
                        []() { ftxui::animation::RequestAnimationFrame(); });
                break;
            case SPOCLI::Event::Pause:
                api.PausePlayback(
                        []() { ftxui::animation::RequestAnimationFrame(); });
                break;
            case SPOCLI::Event::SetVolumeUp: {
                auto guard = api.spotifyData.currentlyPlayTrack.read();
                if (guard.value.device.supports_volume) {
                    auto volume = guard.value.device.volume_percent;
                    api.setPlaybackVolume(volume + 10);
                }
            } break;
            case SPOCLI::Event::SetVolumeDown: {
                auto guard = api.spotifyData.currentlyPlayTrack.read();
                if (guard.value.device.supports_volume) {
                    auto volume = guard.value.device.volume_percent;
                    api.setPlaybackVolume(volume - 10);
                }
            } break;
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

        timer.expires_after(std::chrono::milliseconds(100));
        auto [ec] = co_await timer.async_wait(
                boost::asio::as_tuple(boost::asio::use_awaitable));
        if (ec)
            co_return;
    }
}
