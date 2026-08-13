module;
export module ui;
export import ui.cover;

import ftxui;
import concurrentqueue;
import std;
import spotifyWebAPI;
import event;

using namespace ftxui;

export class Ui {
public:
    void render(moodycamel::ConcurrentQueue<SPOCLI::Event> &queue, std::shared_ptr<SpotifyData> spotifyData);
    void renderLogin(moodycamel::ConcurrentQueue<SPOCLI::Event> &queue, std::shared_ptr<SpotifyData> spotifyData);

private:
    moodycamel::ConcurrentQueue<int> q;
    App screen = App::FullscreenAlternateScreen();
};
