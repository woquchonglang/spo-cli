#include "client/client.hpp"
#include "config/config.hpp"
#include "ui/ui.hpp"

int main() {
  Config config;

  Client client(config);
  client.login();

  // Ui ui;
  // ui.render();
}
