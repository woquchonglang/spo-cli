#include <boost/asio.hpp>
import config;
import spotifyWebAPI;
import ui;
import std;
import ftxui;
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

    Ui ui;
    Log::instance().info("ui visualizer init");

    Librespot librespot(ioc.get_executor(), spotifyData);
    Log::instance().info("librespot init");

    AsyncEventHandler eventHandler(ioc, config, spotifyData, librespot);

    eventHandler.scheduleExitHandler([&librespot] { librespot.stop(); });
    boost::asio::co_spawn(
            ioc,
            [&ioc, &eventHandler]() -> boost::asio::awaitable<void> {
                co_await eventHandler.handle();
            },
            boost::asio::detached);
    Log::instance().info("eventHandler init");



    eventHandler.scheduleExitHandler(
            [&eventThread] { eventThread.request_stop(); });


    std::jthread uiThread([&ui, &spotifyData]() { ui.render(spotifyData); });

    eventHandler.scheduleExitHandler([&ioc] { ioc.stop(); });
    Log::instance().info("cororuntime init ");

    ioc.run();

}
