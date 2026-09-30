#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

import config;
import spotifyAuth;
import spotifyWebAPI;
import ui;
import std;
import ftxui;
import httplib;
import event;
import spsc;
import librespot;

int main() {
    boost::asio::io_context ioc;

    Config config;
    SpotifyData spotifyData;
    SpotifyAuth auth(config);
    Ui ui;
    Librespot librespot(ioc.get_executor(), spotifyData);

    EventHandler eventHandler(auth, spotifyData, librespot);

    eventHandler.scheduleExitHandler([&librespot] { librespot.stop(); });

    std::jthread eventThread(
            [&eventHandler](std::stop_token st) { eventHandler.handle(st); });

    eventHandler.scheduleExitHandler(
            [&eventThread] { eventThread.request_stop(); });


    std::jthread uiThread([&ui, &spotifyData]() { ui.render(spotifyData); });


    ioc.run();

    eventThread.join();
}
