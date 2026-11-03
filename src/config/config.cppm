module;
export module config;

import std;
import tomlplusplus;
import config.data;
import utils.cow;

namespace config {

namespace fs = std::filesystem;

export class Config {
public:
    Config();
    ~Config();
    std::expected<Data, std::string> parse(std::string_view config_path);
    void refresh();
    void register_ui_notify_msg_cb(std::function<void(std::string)> cb);

    Cow<config::Data> data;

    fs::path config_path;
    fs::path config_directory;
    fs::path cache_path;

    std::function<void(std::string)> ui_notify_msg_cb;
};

}
