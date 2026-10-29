module;
#include <boost/asio/awaitable.hpp>
module event;

import ftxui;
import mpris;
import magic_enum;
import log;

AsyncEventHandler::AsyncEventHandler(asio::io_context &ioc,
                                     SpotifyClient &client, Ui &ui)
        : client(client), ioc(ioc), ui(ui) {};

asio::awaitable<void> AsyncEventHandler::retry_get_devices() {
    constexpr int MAX_RETRIES = 5;

    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        Log::instance().info("Attempting to get available devices, attempt {}",
                             attempt + 1);
        co_await client.api.AsyncGetAvailableDevices();

        bool has_devices = false;
        {
            auto guard = client.api.spotifyData.devices.read();
            has_devices = !guard.value.empty();
        }

        if (has_devices) {
            co_await client.api.TransferPlayback(true);
            co_await client.api.GetCurrentlyPlayingTrack();
            ftxui::animation::RequestAnimationFrame();
            client.mpris->notify_channel.try_send(boost::system::error_code{},
                                                  MpricEvent::all_update);
            Log::instance().info("Successfully got available devices");
            co_return;
        }

        asio::steady_timer timer(co_await asio::this_coro::executor);
        timer.expires_after(std::chrono::seconds(6));
        co_await timer.async_wait(asio::use_awaitable);
    }

    for (auto &handler : exitHandler) {
        handler();
    }
}

asio::awaitable<void> AsyncEventHandler::handle() {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    while (true) {
        SPOCLI::Event event;
        if (eventQueue.try_dequeue(event)) {
            switch (event) {
            case SPOCLI::Event::Login: {
                asio::co_spawn(
                        ioc,
                        [this]() -> asio::awaitable<void> {
                            if (co_await client.auth.login()) {
                                client.api.updateAccessToken(
                                        client.auth.getAccessToken());
                                client.api.spotifyData.isLogined.write().value =
                                        true; // INFO: DEBUG
                                co_await retry_get_devices();
                            }
                        },
                        asio::detached);
            } break;
            case SPOCLI::Event::GetUserProfile:
                client.api.getUserProfile([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    auto data = client.data.userProfile.read().value;
                    auto key = magic_enum::enum_name(
                            SPOCLI::Event::GetUserProfile);
                    client.cache.saveToCache(key.data(), data);
                });

                break;
            case SPOCLI::Event::GetUserTopArtists:
                client.api.getUserTopArtists([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    auto data = client.data.topArtists.read().value;
                    auto key = magic_enum::enum_name(
                            SPOCLI::Event::GetUserTopArtists);
                    client.cache.saveToCache(key.data(), data);
                });

                break;
            case SPOCLI::Event::GetUserTopTracks:
                client.api.getUserTopTracks([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    auto data = client.data.topTracks.read().value;
                    auto key = magic_enum::enum_name(
                            SPOCLI::Event::GetUserTopTracks);
                    client.cache.saveToCache(key.data(), data);
                });
                break;
            case SPOCLI::Event::GetUserFollowedPlaylist:
                break;
            case SPOCLI::Event::GetUserPlaylists:
                client.api.GetUserPlaylists([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    auto data = client.data.userPlaylists.read().value;
                    auto key = magic_enum::enum_name(
                            SPOCLI::Event::GetUserPlaylists);
                    client.cache.saveToCache(key.data(), data);
                });

                break;
                // player
            case SPOCLI::Event::GetCurrentlyPlayingTrack:
                client.api.GetCurrentlyPlayingTrack([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
                });
                break;
            case SPOCLI::Event::GetAvailableDevice:
                client.api.GetAvailableDevices(nullptr);
                break;
            case SPOCLI::Event::GetUserQueue:
                client.api.GetUserQueue(
                        []() { ftxui::animation::RequestAnimationFrame(); });
                break;
            case SPOCLI::Event::SkipToNext: {
                client.api.skipToNext([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
                });
            } break;
            case SPOCLI::Event::SkipToPrevious: {
                client.api.skipToPrevious([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
                });
            } break;
            case SPOCLI::Event::SeekRight: {
                auto guard1 = client.api.spotifyData.currentlyPlayTrack.read();
                client.api.SeekPosition(
                        [this]() {
                            ftxui::animation::RequestAnimationFrame();
                            client.mpris->notify_channel.try_send(
                                    boost::system::error_code{},
                                    MpricEvent::all_update);
                        },
                        guard1.value.progress_ms + 300);
            } break;
            case SPOCLI::Event::SeekLeft: {
                auto guard1 = client.api.spotifyData.currentlyPlayTrack.read();
                client.api.SeekPosition(
                        [this]() {
                            ftxui::animation::RequestAnimationFrame();
                            client.mpris->notify_channel.try_send(
                                    boost::system::error_code{},
                                    MpricEvent::all_update);
                        },
                        guard1.value.progress_ms - 300);
            } break;
            case SPOCLI::Event::Play:
            case SPOCLI::Event::Resume:
                client.api.ResumePlayback([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
                });
                break;
            case SPOCLI::Event::Pause:
                client.api.PausePlayback([this]() {
                    ftxui::animation::RequestAnimationFrame();
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
                });
                break;
            case SPOCLI::Event::SetVolumeUp: {
                auto guard = client.api.spotifyData.currentlyPlayTrack.read();
                if (guard.value.device.supports_volume) {
                    auto volume = guard.value.device.volume_percent;
                    client.api.setPlaybackVolume(volume + 10);
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
                }
            } break;
            case SPOCLI::Event::SetVolumeDown: {
                auto guard = client.api.spotifyData.currentlyPlayTrack.read();
                if (guard.value.device.supports_volume) {
                    auto volume = guard.value.device.volume_percent;
                    client.api.setPlaybackVolume(volume - 10);
                    client.mpris->notify_channel.try_send(
                            boost::system::error_code{},
                            MpricEvent::all_update);
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
            case SPOCLI::Event::NextTrack:
                asio::co_spawn(
                        ioc,
                        [this]() -> asio::awaitable<void> {
                            co_await client.api.GetCurrentlyPlayingTrack();
                            ui.screen.PostEvent(Event_NextTrack);
                        },
                        asio::detached);
                break;
            case SPOCLI::Event::GetLyrics:
                client.librespot.update_lyrics();
                break;
            default:
                break;
            }
        }

        std::string uri;
        if (play_song_spsc.try_dequeue(uri)) {
            client.api.StartPlayback(ftxui::animation::RequestAnimationFrame,
                                     uri);
            client.mpris->notify_channel.try_send(boost::system::error_code{},
                                                  MpricEvent::all_update);
        }

        std::string item;
        if (playlistItems_spsc.try_dequeue(item)) {
            asio::co_spawn(
                    ioc,
                    [this, item]() -> asio::awaitable<void> {
                        co_await client.api.GetUserPlaylistItems(item);
                        ftxui::animation::RequestAnimationFrame();
                    },
                    asio::detached);
        }

        timer.expires_after(std::chrono::milliseconds(100));
        auto [ec] =
                co_await timer.async_wait(asio::as_tuple(asio::use_awaitable));
        if (ec)
            co_return;
    }
}
