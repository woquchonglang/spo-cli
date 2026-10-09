module;
export module config;

import std;
import tomlplusplus;
import rwlock;
import config.data;

namespace fs = std::filesystem;

export class Config {
public:
    Config();
    ~Config();
    std::optional<ConfigData> parse(std::string_view config_path);
    void refresh();

    RwLock<ConfigData> data;

    fs::path config_path;
    fs::path config_directory;
};
