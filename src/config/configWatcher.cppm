module;
export module config.watcher;

import std;
import asio;

namespace config {

export class Watcher {
public:
    Watcher(const std::string &path, asio::io_context &ioc);
    ~Watcher();
    asio::awaitable<void> watch_inotify();
    void setOnChange(std::function<void()> func);

private:
    asio::posix::stream_descriptor desc;
    std::string file;
    int inotify_fd;
    int wd = -1;
    std::function<void()> handle_event;
};

}
