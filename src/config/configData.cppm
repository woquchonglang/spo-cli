module;
export module config.data;

import std;
import ftxui;

export {
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
        std::string login_redirect_url;

        std::string soloist_api_key;

        std::string cache_path;

        bool mpris_enable;

        bool proc_enable;

        bool log_enable;

        // theme
        Component_theme theme;
    };
}
