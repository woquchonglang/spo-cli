import std;
import plugin.api;

using namespace plugin;

class MyPlugin : public plugin::IPlugin {
public:
  MyPlugin() {
    name = "my_plugin";
    author = "woquchonglang";
    version = "0.0.1";
    abi_version = "0.10.0";
  }
  void Init() {
    std::println("my plugin init: version: {}, abi_version: {}", version,
                 abi_version);
  }
  void Run() { std::println("my plugin run"); };
  void Shutdown() { std::println("my plugin shutdown"); };
};

extern "C" IPlugin *create_plugin() {
  MyPlugin *my_plugin = new MyPlugin;
  return my_plugin;
}
