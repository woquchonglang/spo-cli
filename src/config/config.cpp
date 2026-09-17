module;
module config;

import tomlplusplus;

using namespace std::literals;

Config::Config() {
    const auto path = "default.toml"sv;

    toml::table config = toml::parse_file(path);
    // std::cout << tbl << "\n";

    client_id = config["spotify"]["client_id"].value_or(""sv);
    client_secret = config["spotify"]["client_secret"].value_or(""sv);
    login_redirect_url = config["spotify"]["login_redirect_url"].value_or(""sv);
    cache_path = config["cache"]["path"].value_or("$HOME/.config/spo-cli"sv);
}

Config::~Config() {}
