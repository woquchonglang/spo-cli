module;
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include "librespot.rs.h"
module librespot;

struct Librespot::Impl {
    rust::Box<librespot::SpotifyContext> ctx;
    Impl() : ctx(librespot::play_backends()) {}
};

Librespot::Librespot(net::any_io_executor exec, SpotifyData &spotifyData)
        : pimpl(std::make_unique<Impl>()), data(spotifyData), exec(exec) {}

Librespot::~Librespot() { stop(); };

void Librespot::update_lyrics() {
    auto id = data.currentlyPlayTrack.read().value.currently_playing.id;

    std::thread([this, id]() {
        auto vec = librespot::get_lyrics_lines(*pimpl->ctx, id);

        auto w = data.curLyric.write();
        w.value.clear();
        w.value.reserve(vec.size());
        for (const auto &line : vec) {
            w.value.push_back({ line.time_ms, std::string(line.text) });
        }
    }).detach();
}

void Librespot::stop() { librespot::cancel_play_backends(); }
