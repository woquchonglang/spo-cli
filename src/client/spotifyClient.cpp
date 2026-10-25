module;
#include <rfl/json.hpp>
#include <rfl.hpp>
module spotify;

import magic_enum;
import event;
import log;

SpotifyClient::SpotifyClient(asio::io_context &ioc,
                             const config::Config &config,
                             asio::thread_pool &pool)
        : api(ioc, nullptr, data)
        , auth(ioc, config)
        , cache()
        , librespot(pool, data) {
    initDataFromCache();

    librespot.start();
    Log::instance().info("librespot init");
}

void SpotifyClient::initDataFromCache() {
    using SPOCLI::Event;
    auto name = magic_enum::enum_name(Event::GetUserProfile);
    auto json = cache.getFromCache(name.data());
    if (json.has_value()) {
        auto _data = rfl::json::read<UserProfile>(json.value()).value();
        data.userProfile.write().value = _data;
    }

    name = magic_enum::enum_name(Event::GetUserTopArtists);
    json = cache.getFromCache(name.data());
    if (json.has_value()) {
        auto _data = rfl::json::read<UserTopArtistsData>(json.value()).value();
        data.topArtists.write().value = _data;
    }

    name = magic_enum::enum_name(Event::GetUserTopTracks);
    json = cache.getFromCache(name.data());
    if (json.has_value()) {
        auto _data = rfl::json::read<UserTopTracksData>(json.value()).value();
        data.topTracks.write().value = _data;
    }

    name = magic_enum::enum_name(Event::GetUserPlaylists);
    json = cache.getFromCache(name.data());
    if (json.has_value()) {
        auto _data = rfl::json::read<UserPlaylistsData>(json.value()).value();
        data.userPlaylists.write().value = _data;
    }
}
