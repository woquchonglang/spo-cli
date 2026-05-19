module;
export module spotifyWebAPI;

import httplib;
import nlohmann.json;
import std;

export class SpotifyWebAPI {
public:
    SpotifyWebAPI(const std::string &accessToken);

    void getProfile();

private:
    std::string accessToken;
};

SpotifyWebAPI::SpotifyWebAPI(const std::string &accessToken) : accessToken(accessToken) {};

void SpotifyWebAPI::getProfile() {
    httplib::Client client("https://api.spotify.com");
    std::string authHeader = "Bearer " + accessToken;
    httplib::Headers headers = { { "Authorization", authHeader } };

    auto res = client.Get("/v1/me", headers);

    if (res && res->status == 200) {
        nlohmann::json response = nlohmann::json::parse(res->body);
        std::string id = response["id"];
        std::string displayName = response["display_name"];
        std::string email = response["email"];
        std::print("User ID: {}\n", id);
        std::print("Display Name: {}\n", displayName);
        std::print("Email: {}\n", email);
    } else {
        std::cerr << "HTTP request failed, error code: " << static_cast<int>(res.error()) << std::endl;
    }
}
