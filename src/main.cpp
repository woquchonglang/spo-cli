#include <boost/asio.hpp>
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
import log;

int main() {
    boost::asio::io_context ioc;

    Config config;
    Log::instance().info("config init");
    boost::asio::co_spawn(
    boost::asio::co_spawn(
            ioc,
            [&ioc]() -> boost::asio::awaitable<void> {
                co_await Proc::Info::instance().sample(ioc);
            },
            boost::asio::detached);
    Log::instance().info("proc init");

    SpotifyData spotifyData;
    SpotifyAuth auth(config);
    Log::instance().info("spotify auth init");

    Ui ui;
    Log::instance().info("ui visualizer init");

    Librespot librespot(ioc.get_executor(), spotifyData);
    Log::instance().info("librespot init");

    EventHandler eventHandler(auth, spotifyData, librespot);

    eventHandler.scheduleExitHandler([&librespot] { librespot.stop(); });

    Log::instance().info("eventHandler init");


    std::jthread eventThread(
            [&eventHandler](std::stop_token st) { eventHandler.handle(st); });

    eventHandler.scheduleExitHandler(
            [&eventThread] { eventThread.request_stop(); });


    std::jthread uiThread([&ui, &spotifyData]() { ui.render(spotifyData); });

    eventHandler.scheduleExitHandler([&ioc] { ioc.stop(); });
    Log::instance().info("cororuntime init ");

    ioc.run();

    eventThread.join();
}
