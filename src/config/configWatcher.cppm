module;
#include <boost/asio.hpp>
export module config.watcher;

import std;

export class ConfigWatcher {
public:
    ConfigWatcher(const std::string &path, boost::asio::io_context &ioc);
    ~ConfigWatcher();
    boost::asio::awaitable<void> watch_inotify();
    void setOnChange(std::function<void()> func);

private:
    boost::asio::posix::stream_descriptor desc;
    std::string file;
    int inotify_fd;
    int wd = -1;
    std::function<void()> handle_event;
};
