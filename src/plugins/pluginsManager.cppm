module;
export module plugin;

import plugin.api;
import std;
import asio;

export namespace plugin {

struct CPlugin {
    IPlugin *iplugin;
    void *handle;
};

class Manager {
public:
    Manager(const std::string &path, asio::io_context &ioc);
    void load_plugin(std::string_view name);
    void unLoad_plugin(std::string_view name);
    asio::awaitable<void> watch_inotify();
    void check_plugin();
    void init_plugins();
    void run_plugins();
    void shutdown_plugins();

private:
    std::vector<CPlugin> plugins;
    asio::posix::stream_descriptor desc;
    int inotify_fd;
    int wd = -1;
};

}
