module;
#include <dlfcn.h>
#include <errno.h>
#include <sys/inotify.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
module plugin;

import log;

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

using namespace plugin;
using namespace boost::asio::experimental::awaitable_operators;

Manager::Manager(const std::string &path, asio::io_context &ioc)
        : inotify_fd(inotify_init1(IN_NONBLOCK | IN_CLOEXEC)), desc(ioc) {
    if (inotify_fd < 0) {
        Log::instance().error("inotify_init1 failed: {}", std::strerror(errno));
        return;
    }
    desc.assign(inotify_fd);
    const uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE;
    wd = inotify_add_watch(inotify_fd, path.c_str(), mask);
    if (wd < 0) {
        Log::instance().error("inotify_add_watch failed for {}: {}", path,
                              std::strerror(errno));
        return;
    }
}

void Manager::load_plugin(std::string_view name) {
    void *handle = dlopen(name.data(), RTLD_NOW);
    if (!handle) {
        std::string strerr = dlerror();
        return;
    }

    dlerror();
    void *sym = dlsym(handle, "create_plugin");
    const char *err = dlerror();
    if (err || !sym) {
        Log::instance().error("plugin load failed: {}", err);
        return;
    }
    auto create_plugin = reinterpret_cast<IPlugin *(*)()>(sym);
    IPlugin *plugin = create_plugin();

    if (plugin->name.empty() || plugin->version.empty() ||
        plugin->author.empty()) {
        Log::instance().error("plugin load failed: some contents empty");
        return;
    }

    if (plugin->abi_version != GIT_VERSION) {
        Log::instance().error("plugin load failed: abi version not matching");
        return;
    }
    CPlugin cplugin{ .iplugin = plugin, .handle = handle };
    plugins.emplace_back(cplugin);
    Log::instance().info("plugin load: success");

    return;
}

void Manager::unLoad_plugin(std::string_view name) {
    for (auto it = plugins.begin(); it != plugins.end();) {
        if (!(*it).iplugin)
            return;

        if ((*it).iplugin->name == name) {
            if ((*it).handle) {
                void *_handle = (*it).handle;
                it = plugins.erase(it);
                dlclose(_handle);
                Log::instance().info("plugin unload: success");
                return;
            }
        } else {
            ++it;
        }
    }
}

asio::awaitable<void> Manager::watch_inotify() {
    constexpr size_t BUF_SIZE = sizeof(inotify_event) + NAME_MAX + 1;
    alignas(inotify_event) char buf[BUF_SIZE];
    for (;;) {
        size_t n = co_await desc.async_read_some(asio::buffer(buf),
                                                 asio::use_awaitable);
        if (n == 0)
            continue;
        size_t offset = 0;
        while (offset < n) {
            auto *ev = reinterpret_cast<inotify_event *>(buf + offset);
            offset += sizeof(inotify_event) + ev->len;
            std::string name = ev->len ? ev->name : "";
            if (ev->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) {
                load_plugin(name);
            } else if (ev->mask & IN_DELETE) {
                unLoad_plugin(name);
            }
        }
    }
    Log::instance().info("ConfigWatcher: watch_inotify exit");
}
