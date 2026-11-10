module;
export module plugin.api;

import std;

export namespace plugin {

// need to implement
// build: g++ -fPIC -shared Plugin.cpp -o Plugin.so
class IPlugin {
public:
    virtual void init() = 0;
    virtual void shutdown() = 0;

public:
    std::string name;
    std::string author;
    std::string version;
    std::string abi_version;
};

extern "C" IPlugin *create_plugin();

// useful functions

bool add_notification(const std::string &text, const float timeMs);

template <typename T>
using RefArg =
        std::conditional_t<std::is_trivially_copyable_v<T>, T, const T &>;

template <typename... Args>
void listen(std::function<void(RefArg<Args>...)> func);

}
