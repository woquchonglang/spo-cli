module;
#include <stdlib.h>
module config;

import ftxui;
import :defaults;
import magic_enum;
import log;

using namespace std::literals; // for ""sv

namespace magic_enum::customize {
template <> struct enum_range<ftxui::Color::Palette256> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
}

ftxui::Color string_to_color(const std::string &name) {
    if (auto c = magic_enum::enum_cast<ftxui::Color::Palette1>(name)) {
        return ftxui::Color(*c);
    }
    if (auto c = magic_enum::enum_cast<ftxui::Color::Palette16>(name)) {
        return ftxui::Color(*c);
    }
    if (auto c = magic_enum::enum_cast<ftxui::Color::Palette256>(name)) {
        return ftxui::Color(*c);
    }
    return ftxui::Color::Default;
}

Config::Config() {
    // current project test
    config_directory = std::filesystem::path(".config");
    config_path = config_directory / "spocli.toml";
    if (fs::exists(config_path)) {
        auto new_config = parse(config_path.string());
        if (new_config.has_value()) {
            data.write(std::make_shared<ConfigData>(std::move(*new_config)));
            return;
        }
    }

    // application config path
    config_directory =
            std::filesystem::path(getenv("HOME")) / ".config" / "spo-cli";
    if (!fs::exists(config_directory)) {
        std::filesystem::create_directories(config_directory);
    }

    config_path = config_directory / "spocli.toml";
    if (std::filesystem::exists(config_path)) {
        auto user_config = parse(config_path.string());
        if (user_config.has_value()) {
            data.write(std::make_shared<ConfigData>(std::move(*user_config)));
            return;
        } else {
            std::println("error: {}", user_config.error());
            throw;
        }
    }
    DefaultConfig default_config(config_path);
    data.write(std::make_shared<ConfigData>(std::move(default_config.get())));
    return;
}

Config::~Config() {}

std::expected<ConfigData, std::string>
Config::parse(std::string_view config_path) {
    toml::parse_result config = toml::parse_file(config_path);
    if (!config) {
        auto error = config.error();
        Log::instance().error(
                "config parse failed {} in {}:{}", error.description(),
                error.source().path ? *error.source().path : "<unknown>",
                error.source().begin.line);
        std::string _notify_msg = std::format(
                "config error in {}:{}",
                error.source().path ? *error.source().path : "<unknown>",
                error.source().begin.line);
        notify_msg.write(std::make_shared<std::string>(_notify_msg));
        return std::unexpected(_notify_msg);
    }
    notify_msg.write(std::make_shared<std::string>(std::move("")));

    ConfigData cfg;

    cfg.client_id = config["spotify"]["client_id"].value_or(""sv);
    cfg.login_redirect_url =
            config["spotify"]["login_redirect_url"].value_or(""sv);
    cfg.soloist_api_key = config["soloist"]["api_key"].value_or(""sv);
    cfg.cache_path =
            config["cache"]["path"].value_or("$HOME/.config/spo-cli"sv);

    cfg.mpris_enable = config["mpris"]["enable"].value_or(false);

    cfg.proc_enable = config["proc"]["enable"].value_or(false);

    cfg.log_enable = config["log"]["enable"].value_or(false);

    // theme
    cfg.preset = config["theme"]["preset"].value_or(""sv);

    cfg.theme.progressline_fg = string_to_color(
            config["theme"]["progressline"]["fg"].value_or("Default"));
    cfg.theme.progressline_bg = string_to_color(
            config["theme"]["progressline"]["bg"].value_or("Default"));
    cfg.theme.visualizer_high = string_to_color(
            config["theme"]["visualizer"]["high"].value_or("Default"));
    cfg.theme.visualizer_low = string_to_color(
            config["theme"]["visualizer"]["low"].value_or("Default"));
    cfg.theme.lyrics_fg =
            string_to_color(config["theme"]["lyrics"]["fg"].value_or("White"));
    cfg.theme.lyrics_bg = string_to_color(
            config["theme"]["lyrics"]["bg"].value_or("GrayDark"));

    return cfg;
}

void Config::refresh() {
    auto new_config = parse(config_path.string());
    if (new_config.has_value()) {
        data.write(std::make_shared<ConfigData>(std::move(*new_config)));
    } else {
        std::cerr << "Failed to refresh config from " << config_path
                  << std::endl;
    }
}
