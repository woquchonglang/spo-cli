module;
export module config;

import std;
import tomlplusplus;
import ftxui;
import rwlock;

using namespace ftxui;
struct Component_theme {
    Color progressline_fg;
    Color progressline_bg;
    Color visualizer_high;
    Color visualizer_low;
    Color lyrics_fg;
    Color lyrics_bg;
};

struct ConfigData {
    std::string client_id;
    std::string client_secret;
    std::string login_redirect_url;

    std::string soloist_api_key;

    std::string cache_path;

    bool mpris_enable;

    bool proc_enable;

    bool log_enable;

    // theme
    Component_theme theme;
};

export class Config {
public:
    Config();
    ~Config();
    std::optional<ConfigData> parse(std::string_view config_path);
    void refresh();

    RwLock<ConfigData> data;

    std::string config_path;
    std::string config_directory;
};
