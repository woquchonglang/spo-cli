module;
module config;

using namespace std::literals;

Config::Config() {
    config_directory = std::filesystem::path("config");
    config_path = config_directory + "/default.toml";
    auto new_config = parse(config_path);
    if (new_config.has_value()) {
        data.write().value = std::move(new_config.value());
    } else {
        std::cerr << "Failed to load config from " << config_path << std::endl;
    }
}

Config::~Config() {}

std::optional<ConfigData> Config::parse(std::string_view config_path) {
    toml::parse_result config = toml::parse_file(config_path);
    if (!config) {
        auto error = config.error();
        std::cerr << "config parse failed: " << error.description()
                  << std::endl;
        return std::nullopt;
    }

    ConfigData cfg;

    cfg.client_id = config["spotify"]["client_id"].value_or(""sv);
    cfg.client_secret = config["spotify"]["client_secret"].value_or(""sv);
    cfg.login_redirect_url =
            config["spotify"]["login_redirect_url"].value_or(""sv);
    cfg.soloist_api_key = config["soloist"]["api_key"].value_or(""sv);
    cfg.cache_path =
            config["cache"]["path"].value_or("$HOME/.config/spo-cli"sv);

    cfg.mpris_enable = config["mpris"]["enable"].value_or(false);

    cfg.proc_enable = config["proc"]["enable"].value_or(false);

    cfg.log_enable = config["log"]["enable"].value_or(false);

    // theme
    using enum Color::Palette16;
    using enum Color::Palette256;
    cfg.theme.progressline_fg =
            config["theme"]["progressline"]["fg"].value_or(Green);
    cfg.theme.progressline_bg =
            config["theme"]["progressline"]["bg"].value_or(GrayDark);
    cfg.theme.visualizer_high =
            config["theme"]["visualizer"]["high"].value_or(Red);
    cfg.theme.visualizer_low =
            config["theme"]["visualizer"]["low"].value_or(Orange3);
    cfg.theme.lyrics_fg = config["theme"]["lyrics"]["fg"].value_or(White);
    cfg.theme.lyrics_bg = config["theme"]["lyrics"]["bg"].value_or(GrayDark);

    return cfg;
}

void Config::refresh() {
    auto new_config = parse(config_path);
    if (new_config.has_value()) {
        data.write().value = std::move(new_config.value());
    } else {
        std::cerr << "Failed to refresh config from " << config_path
                  << std::endl;
    }
}
