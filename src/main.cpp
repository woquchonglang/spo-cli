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
import helper;
import plugin;
import plugin;

int main(int argc, const char *argv[]) {
    {
        auto argvs = std::views::counted(std::next(argv), argc - 1) |
                     std::ranges::to<std::vector<std::string_view>>();

        Helper helper;
        if (auto ex = helper.parse(argvs); !ex) {
            return -1;
        }
    };

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

    plugin::Manager plugin_manager(config.plugin_directory, ioc);

    asio::co_spawn(
            ioc,
            [&ioc, &config]() -> asio::awaitable<void> {
                Log::instance().info("proc init");
                co_await Proc::Info::instance().sample(ioc);
            },
            asio::detached);

    SpotifyClient client(ioc, config, backend_thread);

    Ui ui(config, client.data, ioc);
    watcher.setOnChange([&config, &ui] {
        config.refresh();
        ui.screen.PostEvent(Event_ReConfig);
        if (config.data.read()->proc.enable) {
            Proc::Info::instance().enable();
        } else {
            Proc::Info::instance().disable();
        }
    });
    asio::co_spawn(
            ioc,
            [&ioc, &ui]() -> asio::awaitable<void> {
                Log::instance().info("ui init ");
                co_await ui.render();
            },
            asio::detached);

    config.register_ui_notify_msg_cb(
            [&ui](std::string msg) { ui.send_notify_msg(msg); });

    AsyncEventHandler eventHandler(ioc, client, ui);
    eventHandler.scheduleExitHandler([&client] { client.librespot.stop(); });
    asio::co_spawn(
            ioc,
            [&ioc, &eventHandler]() -> asio::awaitable<void> {
                Log::instance().info("eventHandler init");
                co_await eventHandler.handle();
            },
            asio::detached);

    eventHandler.scheduleExitHandler([&ioc] { ioc.stop(); });
    Log::instance().info("coroutine init ");
    ioc.run();
    backend_thread.join();
}
