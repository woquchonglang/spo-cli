module;
#include <boost/asio/awaitable.hpp>
module ui;

import ftxui;
import mpmc;
import std;
import kittyImageComponent;
import spotifyWebAPI;
import event;
import proc;
import :visualizer;
Ui::Ui(Config &config, SpotifyData &spotifyData)
        : screen(App::FullscreenAlternateScreen())
        , config(config)
        , spotifyData(spotifyData) {
    g_visualizer = std::make_unique<Visualizer>();
    g_visualizer->register_cb();


    auto cover = Renderer([&] {
        auto c = Canvas(80, 20);
        for (auto [i, value] : std::views::enumerate(Cover::cover1)) {
            c.DrawText(0, i * 4, value);
        }
        return canvas(std::move(c));
    });

    unsigned int index = 0;
    auto login_text = Renderer([&] {
        return hcenter(
                vbox({ separatorEmpty(), hbox({ text("logging in ") | italic,
                                                spinner(15, index) }) }));
    });

    auto login = Container::Vertical({
                         cover,
                         login_text,
                 }) |
                 center;

    login |= CatchEvent([&](Event event) {
        if (event == Event::q || event == Event::CtrlC) {
            screen.Exit();
            eventQueue.enqueue(SPOCLI::Event::Exit);
            return true;
        }
        return false;
    });

    Loop loginloop(&screen, login);

    asio::steady_timer timer(co_await asio::this_coro::executor);
    while (!spotifyData.isLogined.read().value && main_show) {
        loginloop.RunOnce();
        index++;
        ftxui::animation::RequestAnimationFrame();
        timer.expires_after(std::chrono::milliseconds(100));
        auto [ec] =
                co_await timer.async_wait(asio::as_tuple(asio::use_awaitable));
        if (ec)
            co_return;
    }
}


asio::awaitable<void> Ui::render() {
    };
    int selected = 0;

    MenuOption option;
    // option.on_enter = screen.ExitLoopClosure();
    auto menu = Menu(&entries, &selected, option);


    // auto userProfile = Renderer([&] {
    //     if (spotifyData.userProfile.read().value.has_value()) {
    //         return text("name: " + spotifyData.userProfile.read().value->displayName + "\n" +
    //                     "email: " + spotifyData.userProfile.read().value->email +
    //                     "\n"
    //                     "product: " +
    //                     spotifyData.userProfile.read().value->product);
    //     } else {
    //         queue.enqueue(SPOCLI::Event::GetUserProfile);
    //         return text("Loading...");
    //     }
    // });

    auto userProfile = Renderer([&] {
        return text(
                "name: " + spotifyData.userProfile.read().value.displayName +
                "\n" + "email: " + spotifyData.userProfile.read().value.email +
                "\n"
                "product: " +
                spotifyData.userProfile.read().value.product);
    });

    auto userProfileLoading = Renderer([&] {
        queue.enqueue(SPOCLI::Event::GetUserProfile);
        return text("Loading...") | size(WIDTH, EQUAL, 36);
    });

    auto userTopArtistsTitle = Renderer([&] { return text("Top Artists"); });

    // auto userTopArtists = Renderer([&] {
    //     Elements children = {};
    //     for (auto i : spotifyData.topArtists.read().value->names) {
    //         children.push_back(text(i));
    //     }
    //     return vbox(children);
    // });

    auto userTopArtistsLoading = Renderer([&] {
        queue.enqueue(SPOCLI::Event::GetUserTopArtists);
        return text("Loading...");
    });

    MenuOption userTopArtistsOption;
    int userTopArtistsSelected = 0;
    auto userTopArtistsMenu = [&]() -> Component {
        auto guard = spotifyData.topArtists.read();
        return Menu(&guard.value.names, &userTopArtistsSelected,
                    userTopArtistsOption);
    }();

    MenuOption userTopTracksOption;
    userTopTracksOption.on_enter = [&] {
        // if (spotifyData.topTracks.read().value.tracks.size() > 0) {
        //     queue.enqueue(SPOCLI::Event::PlayTrack(spotifyData.topTracks.read().value.uri[userTopTracksSelected]));
        // }
    };
    int userTopTracksSelected = 0;
    auto userTopTracksMenuComponent = [&]() -> Component {
        auto guard = spotifyData.topTracks.read();
        return Menu(&guard.value.tracks, &userTopArtistsSelected,
                    userTopArtistsOption);
    }();

    auto userTopTracksMenu =
            Renderer(userTopTracksMenuComponent, [&]() -> Element {
                auto guard = spotifyData.topTracks.read();
                if (guard.value.tracks.size() > 0) {
                    return userTopTracksMenuComponent->Render();
                } else {
                    queue.enqueue(SPOCLI::Event::GetUserTopTracks);
                    return text("Loading...") | center;
                }
            });

    auto user_Profile = Renderer([&] {
        Elements children = {};
        auto guard = spotifyData.userProfile.read();
        if (guard.value.account_id != "") {
            children.push_back(userProfile->Render());
        } else {
            children.push_back(userProfileLoading->Render());
        }
        children.push_back(separatorEmpty());
        children.push_back(userTopArtistsTitle->Render());
        children.push_back(separator());
        if (spotifyData.topArtists.read().value.names.size() == 0) {
            queue.enqueue(SPOCLI::Event::GetUserTopArtists);
        }
        return vbox(children);
    });

    co_await renderLogin();

    auto user_home = Container::Vertical(
            { Container::Horizontal({ user_Profile }),
              userTopTracksMenu });

    Loop mainLoop(&screen, renderer);

    asio::steady_timer timer(co_await asio::this_coro::executor);
    while (main_show) {
        updateFrame();
        mainLoop.RunOnce();
        ftxui::animation::RequestAnimationFrame();
        timer.expires_after(std::chrono::milliseconds(16)); // 60fps
        auto [ec] =
                co_await timer.async_wait(asio::as_tuple(asio::use_awaitable));
        if (ec)
            co_return;
    }

    screen.Exit();
    Log::instance().info("ui render exit");
}
    std::vector<std::string> tab_values{
        "Home",
        "User",
    };
    int tab_drawn = 0;
    auto tab_toggle = Toggle(&tab_values, &tab_drawn);

    auto tab_container = Container::Tab({ menu, user_home }, &tab_drawn);
ftxui::Element Ui::render_cava() {
    auto bars = g_visualizer->bars.read().value;
    ftxui::Elements columns;
    for (int i = 0; i < bars.size() / 2; i++) {
        double v = (bars[i] + bars[bars.size() / 2 + i]) /
                   2.0; // left and right bars
        if (v < 0.01)
            v = 0;
        auto inter_color =
                Color::Interpolate(v, config.data.read()->theme.visualizer_high,
                                   config.data.read()->theme.visualizer_low);
        columns.push_back(ftxui::gaugeUp(v) | color(inter_color) |
                          ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 2));
        columns.push_back(ftxui::text("") |
                          ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 1));
    }
    auto hold = vtext("       ");
    columns.push_back(hold);
    return ftxui::hbox(std::move(columns));
}

    auto container = Container::Vertical({ tab_toggle, tab_container });

    std::map<std::string, std::string> help_map = {
        { "?", "help" },
        { "q", "exit app" },
        { "<enter> e", "tree" },
    };

    bool help_show = false;

    auto left_column = Renderer([&] {
        Elements children = {};
        children.push_back(underlined(text("Key:")));
        for (auto i : help_map) {
            children.push_back(text(i.first));
        }
        return vbox(children);
    });

    auto right_column = Renderer([&] {
        Elements children = {};
        children.push_back(underlined(text("Description:")));
        for (auto i : help_map) {
            children.push_back(text(i.second));
        }
        return vbox(children);
    });

    auto help_component = Renderer([&] {
        return color(Color::Blue, hbox({
                                          left_column->Render(),
                                          separatorEmpty(),
                                          right_column->Render(),
                                  }) | border);
    });

    auto maybe_help_component = Maybe(help_component, &help_show);

    // proc info
    auto proc_info = Renderer([&]() -> Element {
        auto cpu_usage =
                Proc::Info::instance().cpu_info.read().value.get_usage();
        std::wstring wcpu = std::format(L"{:.1f}", cpu_usage);
        auto mem_mb =
                Proc::Info::instance().stat_info.read().value.get_memory();
        std::wstring wmem = std::format(L"{:.1f}", mem_mb);
        return text("cpu:" + ::to_string(wcpu) + "%" + " " +
                    "mem:" + ::to_string(wmem) + "M") |
               center;
    });


    // main ui
    auto renderer = Renderer(container, [&] {
        return vbox({ filler(), container->Render() | center,
                      hbox({ filler(), proc_info->Render() }) });
    });


    renderer |= CatchEvent([&](Event event) {
        if (event == Event::q || event == Event::CtrlC) {
            screen.Exit();
            queue.enqueue(SPOCLI::Event::Exit);
            return true;
        } else if (event == Event::Character('?')) {
            help_show = !help_show;
            maybe_help_component->TakeFocus();
            return true;
        }
        return false;
    });


}


void Ui::reConfig() { render_proc(config.data.read()->proc_enable); }
