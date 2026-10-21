module;
#include <pthread.h>
export module utils.threadTool;
import std;

export void set_thread_name(const std::string &name) {
    char buf[16];
    size_t len = std::min(name.size(), size_t{ 15 });
    std::copy_n(name.data(), len, buf);
    buf[len] = '\0';
    pthread_setname_np(pthread_self(), buf);
}
