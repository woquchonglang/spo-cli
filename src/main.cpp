import config;
import ui;
import std;
import ftxui;
import event;
import spsc;
import version;
import mpris;
import config.watcher;
import log;
import proc;
import spotify;
import asio;

int main() {
    asio::io_context ioc;
    asio::thread_pool backend_thread(4);

    Config config;
    Log::instance().info("config init");
    ConfigWatcher watcher(config.config_directory, ioc);
    asio::co_spawn(
            ioc,
            [&watcher]() -> asio::awaitable<void> {
                co_await watcher.watch_inotify();
            },
            asio::detached);

    Log::instance().info("config watcher init");

    asio::co_spawn(
            ioc,
            [&ioc]() -> asio::awaitable<void> {
                co_await Proc::Info::instance().sample(ioc);
            },
            asio::detached);
    Log::instance().info("proc init");

    SpotifyClient client(ioc, config, backend_thread);

    Ui ui(config, client.data);
    watcher.setOnChange([&config, &ui] {
        config.refresh();
        ui.screen.PostEvent(Event_ReConfig);
    });
    Log::instance().info("ui visualizer init");

    AsyncEventHandler eventHandler(ioc, client, ui);

    eventHandler.scheduleExitHandler([&client] { client.librespot.stop(); });
    asio::co_spawn(
            ioc,
            [&ioc, &eventHandler]() -> asio::awaitable<void> {
                co_await eventHandler.handle();
            },
            asio::detached);
    Log::instance().info("eventHandler init");

    asio::co_spawn(
            ioc,
            [&ioc, &ui]() -> asio::awaitable<void> { co_await ui.render(); },
            asio::detached);
    Log::instance().info("ui init ");

    eventHandler.scheduleExitHandler([&ioc] { ioc.stop(); });
    Log::instance().info("coroutine init ");

    ioc.run();

    backend_thread.join();
}
