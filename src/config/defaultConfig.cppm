module;
#include <rfl/toml.hpp>
#include <rfl.hpp>
module config:defaults;

import std;
import config.data;
import ftxui;

namespace rfl {
template <> struct Reflector<ftxui::Color> {
    using ReflType = std::string;
    static ReflType from(const ftxui::Color &color) { return "Default"; }
};
}

namespace config {

static constexpr std::string_view SPOTIFY_CLIENT_ID =
        "65b708073fc0480ea92a077233ca87bd";
static constexpr std::string_view NCSPOT_CLIENT_ID =
        "d420a117a32841c2b3474932e49fb54b";

namespace fs = std::filesystem;

class Default {
public:
    Default(fs::path config_path) {
        std::ofstream file(config_path);

        cfg.spotify.client_id = SPOTIFY_CLIENT_ID;
        cfg.spotify.login_redirect_url = "http://127.0.0.1:8989/login";
        cfg.cache.path = "$HOME/.config/spo-cli";

        rfl::toml::save(config_path.string(), cfg);
    }

    config::Data get() { return cfg; }

private:
    config::Data cfg;
};

}
