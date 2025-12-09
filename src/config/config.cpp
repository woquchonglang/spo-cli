#include "config.hpp"
#include "toml++/toml.hpp"
#include <iostream>

using namespace std::literals;

Config::Config() {
  const auto path = "default.toml"sv;

  toml::table config = toml::parse_file(path);
  // std::cout << tbl << "\n";

  client_id = config["client_id"].value_or(""sv);
  login_redirect_url = config["login_redirect_url"].value_or(""sv);

  std::cout << client_id << std::endl;
  std::cout << login_redirect_url << std::endl;
}

Config::~Config() {}
