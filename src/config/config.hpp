#pragma once
#include <string>

class Config {
public:
  Config();
  ~Config();

  std::string client_id;
  std::string login_redirect_url;
};
