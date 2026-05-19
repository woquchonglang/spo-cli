module;
#include "httplib.h"
#include <cstddef>

export module spotifyAuth;

import config;
import nlohmann.json;
import httplib;
import std;

export class SpotifyAuth {
public:
    SpotifyAuth(const Config &config);
    void login();
    std::string getAccessToken() const { return access_token; }

private:
    std::string exchangeCodeForToken(const std::string &code);

private:
    const Config &config;
    std::string access_token;
    std::string refresh_token;
    int expires_in;
};

static std::string generateRandomString(size_t length);

SpotifyAuth::SpotifyAuth(const Config &config) : config(config) {}

std::string SpotifyAuth::exchangeCodeForToken(const std::string &code) {
    httplib::Client client("https://accounts.spotify.com");
    std::string body = "code=" + httplib::encode_uri_component(code) +
                       "&redirect_uri=" + httplib::encode_uri_component(config.login_redirect_url) +
                       "&grant_type=authorization_code";
    client.set_basic_auth(config.client_id, config.client_secret);
    auto res = client.Post("/api/token", body, "application/x-www-form-urlencoded");

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
        this->access_token = response["access_token"];
        this->refresh_token = response["refresh_token"];
        this->expires_in = response["expires_in"];

        std::print("Access Token: {}\n", access_token);
        std::print("Refresh Token: {}\n", refresh_token);
        std::print("expires_in: {}\n", expires_in);

        return access_token;
    } else {
        std::cerr << "HTTP request failed, error code: " << static_cast<int>(res.error()) << std::endl;
        return "";
    }
}

/*
 * https://developer.spotify.com/documentation/web-api/tutorials/code-flow
 */
void SpotifyAuth::login() {
    httplib::Server app;

    auto client_id = config.client_id;
    auto redirect_url = config.login_redirect_url;

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

        std::print("Redirecting to Spotify login: {}\n", _redirect_url.str());
    });

    app.Get("/callback", [&](const httplib::Request &req, httplib::Response &res) {
        auto code = req.get_param_value("code");
        auto state = req.get_param_value("state");

        if (state.empty()) {
            res.status = 400;
            res.set_content("State mismatch", "text/plain");
            return;
        }

        std::string token = exchangeCodeForToken(code);

        res.set_content("Successfully authenticated.", "text/plain");
        app.stop();
    });

    std::print("login to: http://127.0.0.1:8989/login\n");
    app.listen("127.0.0.1", 8989);
}

// https://generate-random.org/strings/cpp
std::string generateRandomString(size_t length) {
    const std::string ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, ALPHANUMERIC.size() - 1);
    std::string random_string;
    random_string.reserve(length);
    for (int i = 0; i < length; ++i) {
        random_string += ALPHANUMERIC[distribution(generator)];
    }
    return random_string;
}
