module;
export module plugin;

import plugin.api;
import std;
import asio;

namespace fs = std::filesystem;

export namespace plugin {

struct CPlugin {
    IPlugin *iplugin;
    void *handle;
    std::string lib_name;
};

class Manager {
public:
    Manager(const fs::path &path, asio::io_context &ioc);
    ~Manager();
    void load_plugin(std::string_view name);
    void unLoad_plugin(std::string_view name);
    asio::awaitable<void> watch_inotify();

private:
    std::vector<CPlugin> plugins;
    asio::posix::stream_descriptor desc;
    const fs::path &plugin_path;
    int inotify_fd;
    int wd = -1;
};

}
