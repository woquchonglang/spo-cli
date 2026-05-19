module;
export module config;

import tomlplusplus;
import std;

export class Config {
public:
    Config();
    ~Config();

    std::string client_id;
    std::string client_secret;
    std::string login_redirect_url;
};

using namespace std::literals;

Config::Config() {
    const auto path = "default.toml"sv;

    toml::table config = toml::parse_file(path);
    // std::cout << tbl << "\n";

    client_id = config["spotify"]["client_id"].value_or(""sv);
    client_secret = config["spotify"]["client_secret"].value_or(""sv);
    login_redirect_url = config["spotify"]["login_redirect_url"].value_or(""sv);
}

Config::~Config() {}
