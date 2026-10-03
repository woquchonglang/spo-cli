module;
export module ui;
export import ui.cover;

import ftxui;
import mpmc;
import std;
import spotifyWebAPI;
import event;

using namespace ftxui;

export class Ui {
public:
    void render(moodycamel::Mpmc<SPOCLI::Event> &queue,
                SpotifyData &spotifyData);
    void renderLogin(moodycamel::Mpmc<SPOCLI::Event> &queue,
                     SpotifyData &spotifyData);

private:
    App screen = App::FullscreenAlternateScreen();
};
