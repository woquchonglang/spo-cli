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

    config::Config config;
    Log::instance().info("config init");
    config::Watcher watcher(config.config_directory, ioc);
    asio::co_spawn(
            ioc,
            [&watcher]() -> asio::awaitable<void> {
                Log::instance().info("config watcher init");
                co_await watcher.watch_inotify();
            },
            asio::detached);

    asio::co_spawn(
            ioc,
            [&ioc, &config]() -> asio::awaitable<void> {
                Log::instance().info("proc init");
                co_await Proc::Info::instance().sample(ioc);
            },
            asio::detached);

    SpotifyClient client(ioc, config, backend_thread);

    Ui ui(config, client.data);
    watcher.setOnChange([&config, &ui] {
        config.refresh();
        ui.screen.PostEvent(Event_ReConfig);
        if (config.data.read()->proc.enable) {
            Proc::Info::instance().enable();
        } else {
            Proc::Info::instance().disable();
        }
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
