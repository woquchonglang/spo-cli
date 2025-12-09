#include "./client.hpp"
#include "httplib.h"
// #include "json.hpp"

Client::Client(const Config &config) : config(config) {}

std::string generateRandomString(size_t length) {
  const char charset[] = "0123456789"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz";
  std::mt19937 generator(std::random_device{}());
  std::uniform_int_distribution<size_t> distribution(0, sizeof(charset) - 2);

  std::string random_string;
  for (size_t i = 0; i < length; ++i) {
    random_string += charset[distribution(generator)];
  }
  return random_string;
}

void Client::login() {

  httplib::Server app;

  app.Get("/login", [&](const httplib::Request &, httplib::Response &res) {
    std::string state = generateRandomString(16);
    std::string scope = "user-read-private user-read-email";

    std::ostringstream _redirect_url;
    _redirect_url << "https://accounts.spotify.com/authorize?"
                  << "response_type=code&"
                  << "client_id=" << config.client_id << "&"
                  << "scope=" << scope << "&"
                  << "redirect_uri=" << config.login_redirect_url << "&"
                  << "state=" << state;

    res.set_redirect(_redirect_url.str());

    std::cout << _redirect_url.str() << std::endl;
  });

  std::cout << "Server is running at http://127.0.0.1:8989" << std::endl;
  app.listen("127.0.0.1", 8989);
}
