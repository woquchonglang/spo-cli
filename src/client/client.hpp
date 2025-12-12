#pragma once

#include "../config/config.hpp"

class Client {
public:
  Client(const Config &config);
  void login();


private:
  std::string exchangeCodeForToken(const std::string &code);

private:
  const Config &config;
};
