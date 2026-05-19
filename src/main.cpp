
import config;
import spotifyAuth;
import spotifyWebAPI;
import std;

int main() {
    Config config;

    SpotifyAuth auth(config);
    auth.login();

    SpotifyWebAPI api(auth.getAccessToken());
    api.getProfile();
}
