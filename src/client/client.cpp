#include "./client.hpp"
#include "curl/curl.h"
#include "httplib.h"

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

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *userp) {
  size_t total_size = size * nmemb;
  userp->append(static_cast<char *>(contents), total_size);
  return total_size;
}

std::string Client::exchangeCodeForToken(const std::string &code) {
  CURL *curl;
  CURLcode res;
  std::string response_data;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (!curl) {
    return "Failed to initialize curl";
  }

  // Prepare POST data
  std::string post_fields =
      "code=" +
      std::string(curl_easy_escape(curl, code.c_str(), code.length())) +
      "&redirect_uri=" +
      curl_easy_escape(curl, config.login_redirect_url.c_str(),
                       config.login_redirect_url.length()) +
      "&grant_type=authorization_code";

  std::string auth_string = config.client_id + ":" + config.client_secret;
  std::string auth_header =
      "Basic " + httplib::detail::base64_encode(auth_string);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(
      headers, "Content-Type: application/x-www-form-urlencoded");
  headers =
      curl_slist_append(headers, ("Authorization: " + auth_header).c_str());

  curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    response_data =
        "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return response_data;
}

/* https://developer.spotify.com/documentation/web-api/tutorials/code-flow */
void Client::login() {

  httplib::Server app;

  auto client_id = config.client_id;
  auto redirect_url = config.login_redirect_url;

  std::cout << client_id << std::endl;
  std::cout << redirect_url << std::endl;

  app.Get("/login", [&](const httplib::Request &, httplib::Response &res) {
    std::string state = generateRandomString(16);
    std::string scope = "user-read-private user-read-email";

    std::ostringstream _redirect_url;
    _redirect_url << "https://accounts.spotify.com/authorize?"
                  << "response_type=code&"
                  << "client_id=" << client_id << "&"
                  << "scope=" << scope << "&"
                  << "redirect_uri=" << redirect_url << "&"
                  << "state=" << state;

    res.set_redirect(_redirect_url.str());

    std::cout << _redirect_url.str() << std::endl;
  });

  app.Get("/callback",
          [&](const httplib::Request &req, httplib::Response &res) {
            auto code = req.get_param_value("code");
            auto state = req.get_param_value("state");

            if (state.empty()) {
              res.status = 400;
              res.set_content("State mismatch", "text/plain");
              return;
            }

            std::string token = exchangeCodeForToken(code);
            std::cout << "Token response: " << token << std::endl;

            res.set_content("Successfully authenticated.", "text/plain");
            app.stop();
          });

  std::cout << "login to: http://127.0.0.1:8989/login" << std::endl;
  app.listen("127.0.0.1", 8989);
}
