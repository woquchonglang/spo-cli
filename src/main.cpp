import config;
import spotifyAuth;
import spotifyWebAPI;
import ui;
import std;
import ftxui;
import httplib;
import event;
import concurrentqueue;

int main() {
    Config config;
    std::shared_ptr<SpotifyData> spotifyData = std::make_shared<SpotifyData>();
    moodycamel::ConcurrentQueue<SPOCLI::Event> eventQueue;
    SpotifyAuth auth(config);

    EventHandler eventHandler(eventQueue, auth, spotifyData);
    std::jthread eventThread([&eventHandler](std::stop_token st) { eventHandler.handle(st); });

    eventHandler.scheduleExitHandler([&eventThread] { eventThread.request_stop(); });

    Ui ui;
    std::jthread uiThread(&Ui::render, &ui, std::ref(eventQueue), spotifyData);

    eventThread.join();
}
