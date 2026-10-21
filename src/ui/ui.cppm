module;
export module ui;

import ftxui;
import mpmc;
import std;
import spotifyWebAPI;
import event;
import config;
import asio;

using namespace ftxui;

export class Ui {
public:
    Ui(Config &config, SpotifyData &spotifyData);
    void reConfig();

    asio::awaitable<void> render();
    asio::awaitable<void> renderLogin();

    Element render_cava();
    void render_curlyrics();
private:
    App screen = App::FullscreenAlternateScreen();
    Component curlyrics;
    Component current_playing_song;
};
