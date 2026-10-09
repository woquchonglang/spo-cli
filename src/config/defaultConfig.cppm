module;
#include <rfl/toml.hpp>
#include <rfl.hpp>
export module config.defaults;

import std;
import config.data;
import ftxui;

static constexpr std::string_view SPOTIFY_CLIENT_ID =
        "65b708073fc0480ea92a077233ca87bd";
static constexpr std::string_view NCSPOT_CLIENT_ID =
        "d420a117a32841c2b3474932e49fb54b";

namespace fs = std::filesystem;

namespace rfl {
template <> struct Reflector<ftxui::Color> {
    using ReflType = std::string;
    static ReflType from(const ftxui::Color &color) { return "Default"; }
};
}

export class DefaultConfig {
public:
    DefaultConfig(fs::path config_path) {
        std::ofstream file(config_path);

        cfg.client_id = SPOTIFY_CLIENT_ID;
        cfg.login_redirect_url = "http://localhost:8989/callback";
        cfg.cache_path = "$HOME/.config/spo-cli";

        rfl::toml::save(config_path.string(), cfg);
    }

    ConfigData get() { return cfg; }

private:
    ConfigData cfg;
};
