module;
export module config.watcher;

import std;
import asio;

export class ConfigWatcher {
public:
    ConfigWatcher(const std::string &path, asio::io_context &ioc);
    ~ConfigWatcher();
    asio::awaitable<void> watch_inotify();
    void setOnChange(std::function<void()> func);

private:
    asio::posix::stream_descriptor desc;
    std::string file;
    int inotify_fd;
    int wd = -1;
    std::function<void()> handle_event;
};
