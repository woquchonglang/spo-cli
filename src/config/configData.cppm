module;
export module config.data;

import std;
import ftxui;

export {
    namespace config {

    struct Spotify {
        std::string client_id;
        std::string login_redirect_url;
    };

    struct Cache {
        std::string path;
    };

    struct Mpris {
        bool enable = false;
    };

    struct Proc {
        bool enable = false;
    };

    struct Log {
        bool enable = false;
    };

    using namespace ftxui;
    struct Component_theme {
        Color progressline_fg;
        Color progressline_bg;
        Color visualizer_high;
        Color visualizer_low;
        Color lyrics_fg;
        Color lyrics_bg;
    };

    struct Data {
        Spotify spotify;
        Cache cache;
        Mpris mpris;
        Proc proc;
        Log log;
        // theme
        std::string preset;
        Component_theme theme;
    };
    }
}
