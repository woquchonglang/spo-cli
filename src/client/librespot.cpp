module;
#include "librespot.rs.h"
module spotify;

import utils.threadTool;
import event;

struct Librespot::Impl {
    std::optional<rust::Box<librespot::SpotifyContext>> ctx;
    asio::strand<asio::thread_pool::executor_type> strand_;
    Impl(asio::thread_pool &pool) : strand_(asio::make_strand(pool)) {};

    void start() {
        asio::post(strand_, [this]() {
            set_thread_name("librespot");
            if (!ctx.has_value()) {
                ctx.emplace(librespot::play_backends());
            }
        });
    }
};

Librespot::Librespot(asio::thread_pool &pool, SpotifyData &spotifyData)
        : pimpl(std::make_unique<Impl>(pool)), data(spotifyData) {}

Librespot::~Librespot() { stop(); };

void Librespot::start() { pimpl->start(); }

void Librespot::update_lyrics() {
    asio::post(pimpl->strand_, [this]() {
        std::string id;
        {
            auto guard = data.currentlyPlayTrack.read();
            if (guard.value.currently_playing.id.empty()) {
                return;
            }
            id = guard.value.currently_playing.id;
        }

        if (pimpl->ctx.has_value()) {
            try {
                auto vec = librespot::get_lyrics_lines(*pimpl->ctx.value(), id);
                auto w = data.curLyric.write();
                w.value.clear();
                w.value.reserve(vec.size());
                for (const auto &line : vec) {
                    w.value.push_back({ line.time_ms, std::string(line.text) });
                }
            } catch (const std::exception &e) {
                Log::instance().error("get_lyrics_lines failed: {}", e.what());
                std::cerr << "get_lyrics_lines failed: " << e.what()
                          << std::endl;
            }
        }
    });
}

void Librespot::stop() { librespot::cancel_play_backends(); }

extern "C" void cpp_get_player_event_channel_cb() {
    eventQueue.enqueue(SPOCLI::Event::GetCurrentlyPlayingTrack);
}