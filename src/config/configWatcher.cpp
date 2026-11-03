module;
#include <sys/inotify.h>
#include <fcntl.h>
#include <limits.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
module config.watcher;

import std;
import log;

using namespace config;
using namespace boost::asio::experimental::awaitable_operators;

Watcher::Watcher(const std::string &path, asio::io_context &ioc)
        : inotify_fd(inotify_init1(IN_NONBLOCK | IN_CLOEXEC)), desc(ioc) {
    if (inotify_fd < 0) {
        Log::instance().error("inotify_init1 failed: {}", std::strerror(errno));
        return;
    }
    desc.assign(inotify_fd);
    const uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO;
    wd = inotify_add_watch(inotify_fd, path.c_str(), mask);
    if (wd < 0) {
        Log::instance().error("inotify_add_watch failed for {}: {}", path,
                              std::strerror(errno));
        return;
    }
}

Watcher::~Watcher() { inotify_rm_watch(inotify_fd, wd); }

void Watcher::setOnChange(std::function<void()> func) { handle_event = func; }

asio::awaitable<void> Watcher::watch_inotify() {
    constexpr size_t BUF_SIZE = sizeof(inotify_event) + NAME_MAX + 1;
    alignas(inotify_event) char buf[BUF_SIZE];
    asio::steady_timer debounce_timer(co_await asio::this_coro::executor);
    bool pending = false;
    for (;;) {
        if (!pending) {
            std::size_t n = co_await desc.async_read_some(asio::buffer(buf),
                                                          asio::use_awaitable);
            if (n == 0)
                continue;
            pending = true;
            debounce_timer.expires_after(std::chrono::milliseconds(150));
        } else {
            auto result = co_await (
                    desc.async_read_some(asio::buffer(buf),
                                         asio::as_tuple(asio::use_awaitable)) ||
                    debounce_timer.async_wait(
                            asio::as_tuple(asio::use_awaitable)));

            if (result.index() == 1) {
                auto [tec] = std::get<1>(result);
                if (tec == asio::error::operation_aborted) {
                    continue;
                }
                if (tec)
                    co_return;
                pending = false;
                handle_event();
                continue;
            } else {
                auto [nec, n] = std::get<0>(result);
                if (nec)
                    co_return;
                if (n == 0)
                    continue;
                debounce_timer.expires_after(std::chrono::milliseconds(150));
            }
        }
    }
    Log::instance().info("ConfigWatcher: watch_inotify exiting");
}
