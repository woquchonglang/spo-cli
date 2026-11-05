module;
export module plugin.api;

import std;

export namespace plugin {

// need to implement
// build: g++ -fPIC -shared Plugin.cpp -o Plugin.so
class IPlugin {
public:
    virtual void Init() = 0;
    virtual void Run() = 0;
    virtual void Shutdown() = 0;

public:
    std::string name;
    std::string author;
    std::string version;
    std::string abi_version;
};

extern "C" IPlugin *create_plugin();

// useful functions

bool add_notification(const std::string &text, const float timeMs);

}
