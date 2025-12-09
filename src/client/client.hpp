#pragma once

#include "../config/config.hpp"

class Client {
public:
  Client(const Config &config);
  void login();

private:
  const Config &config;
};
