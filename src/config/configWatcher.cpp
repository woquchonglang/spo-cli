module;
#include <sys/inotify.h>
#include <fcntl.h>
#include <boost/asio.hpp>
module config.watcher;

import std;
import log;

ConfigWatcher::ConfigWatcher(const std::string &path,
                             boost::asio::io_context &ioc)
        : inotify_fd(inotify_init1(IN_NONBLOCK | IN_CLOEXEC)), desc(ioc) {
    if (inotify_fd < 0) {
        throw std::runtime_error("inotify_init1 failed");
    }
    desc.assign(inotify_fd);
    const uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO;
    wd = inotify_add_watch(inotify_fd, path.c_str(), mask);
    if (wd < 0) {
        throw std::runtime_error("inotify_add_watch failed for " + path);
    }
}

ConfigWatcher::~ConfigWatcher() { inotify_rm_watch(inotify_fd, wd); }

void ConfigWatcher::setOnChange(std::function<void()> func) {
    handle_event = func;
}

boost::asio::awaitable<void> ConfigWatcher::watch_inotify() {
    constexpr size_t BUF_SIZE = sizeof(inotify_event) + NAME_MAX + 1;
    alignas(inotify_event) char buf[BUF_SIZE];
    for (;;) {
        std::size_t n = co_await desc.async_read_some(
                boost::asio::buffer(buf), boost::asio::use_awaitable);
        Log::instance().info("ConfigWatcher: watch_inotify read {} bytes", n);
        std::size_t offset = 0;
        while (offset < n) {
            auto *ev = reinterpret_cast<inotify_event *>(buf + offset);

            handle_event();

            offset += sizeof(inotify_event) + ev->len;
        }
    }
    Log::instance().info("ConfigWatcher: watch_inotify exiting");
}
