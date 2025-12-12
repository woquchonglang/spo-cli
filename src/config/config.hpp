#pragma once
#include <string>

class Config {
public:
  Config();
  ~Config();

  std::string client_id;
  std::string client_secret;
  std::string login_redirect_url;
};
