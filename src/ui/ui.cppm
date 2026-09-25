module;
export module ui;
export import ui.cover;

import ftxui;
import spsc;
import std;
import spotifyWebAPI;
import event;

using namespace ftxui;

export class Ui {
public:
    void render(moodycamel::Spsc<SPOCLI::Event> &queue, std::shared_ptr<SpotifyData> spotifyData);
    void renderLogin(moodycamel::Spsc<SPOCLI::Event> &queue, std::shared_ptr<SpotifyData> spotifyData);

private:
    App screen = App::FullscreenAlternateScreen();
};
