module;
export module config;

import std;
import tomlplusplus;
import config.data;
import utils.cow;

namespace fs = std::filesystem;

export class Config {
public:
    Config();
    ~Config();
    std::expected<ConfigData, std::string> parse(std::string_view config_path);
    void refresh();

    Cow<ConfigData> data;
    Cow<std::string> notify_msg;

    fs::path config_path;
    fs::path config_directory;
};
