module;
module ui;

import ftxui;
import concurrentqueue;
import std;
import kittyImageComponent;
import spotifyWebAPI;
import event;

void Ui::renderLogin(moodycamel::ConcurrentQueue<SPOCLI::Event> &queue, std::shared_ptr<SpotifyData> spotifyData) {
    queue.enqueue(SPOCLI::Event::Login);

    auto cover = Renderer([&] {
        auto c = Canvas(80, 20);
        for (auto [i, value] : std::views::enumerate(Cover::cover1)) {
            c.DrawText(0, i * 4, value);
        }
        return canvas(std::move(c));
    });

    unsigned int index = 0;
    auto login_text = Renderer([&] {
        return hcenter(vbox({ separatorEmpty(), hbox({ text("logging in ") | italic, spinner(15, index) }) }));
    });

    auto login = Container::Vertical({
                         cover,
                         login_text,
                 }) |
                 center;

    login |= CatchEvent([&](Event event) {
        if (event == Event::q || event == Event::CtrlC) {
            screen.Exit();
            queue.enqueue(SPOCLI::Event::Exit);
            return true;
        }
        return false;
    });

    Loop loginloop(&screen, login);

    while (!spotifyData->isLogined.read().value) {
        loginloop.RunOnce();
        index++;
        ftxui::animation::RequestAnimationFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


void Ui::render(moodycamel::ConcurrentQueue<SPOCLI::Event> &queue, std::shared_ptr<SpotifyData> spotifyData) {
    std::vector<std::string> entries = {
        "Top Albums",
        "Top Artists",
        "Top Tracks",
    };
    int selected = 0;

    MenuOption option;
    // option.on_enter = screen.ExitLoopClosure();
    auto menu = Menu(&entries, &selected, option);


    // auto userProfile = Renderer([&] {
    //     if (spotifyData->userProfile.read().value.has_value()) {
    //         return text("name: " + spotifyData->userProfile.read().value->displayName + "\n" +
    //                     "email: " + spotifyData->userProfile.read().value->email +
    //                     "\n"
    //                     "product: " +
    //                     spotifyData->userProfile.read().value->product);
    //     } else {
    //         queue.enqueue(SPOCLI::Event::GetUserProfile);
    //         return text("Loading...");
    //     }
    // });

    auto userProfile = Renderer([&] {
        return text("name: " + spotifyData->userProfile.read().value.displayName + "\n" +
                    "email: " + spotifyData->userProfile.read().value.email +
                    "\n"
                    "product: " +
                    spotifyData->userProfile.read().value.product);
    });

    auto userProfileLoading = Renderer([&] {
        queue.enqueue(SPOCLI::Event::GetUserProfile);
        return text("Loading...") | size(WIDTH, EQUAL, 36);
    });

    auto userTopArtistsTitle = Renderer([&] { return text("Top Artists"); });

    // auto userTopArtists = Renderer([&] {
    //     Elements children = {};
    //     for (auto i : spotifyData->topArtists.read().value->names) {
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
        auto guard = spotifyData->topArtists.read();
        return Menu(&guard.value.names, &userTopArtistsSelected, userTopArtistsOption);
    }();

    MenuOption userTopTracksOption;
    userTopTracksOption.on_enter = [&] {
        // if (spotifyData->topTracks.read().value.tracks.size() > 0) {
        //     queue.enqueue(SPOCLI::Event::PlayTrack(spotifyData->topTracks.read().value.uri[userTopTracksSelected]));
        // }
    };
    int userTopTracksSelected = 0;
    auto userTopTracksMenuComponent = [&]() -> Component {
        auto guard = spotifyData->topTracks.read();
        return Menu(&guard.value.tracks, &userTopArtistsSelected, userTopArtistsOption);
    }();

    auto userTopTracksMenu = Renderer(userTopTracksMenuComponent, [&]() -> Element {
        auto guard = spotifyData->topTracks.read();
        if (guard.value.tracks.size() > 0) {
            return userTopTracksMenuComponent->Render();
        } else {
            queue.enqueue(SPOCLI::Event::GetUserTopTracks);
            return text("Loading...") | center;
        }
    });

    auto user_Profile = Renderer([&] {
        Elements children = {};
        auto guard = spotifyData->userProfile.read();
        if (guard.value.account_id != "") {
            children.push_back(userProfile->Render());
        } else {
            children.push_back(userProfileLoading->Render());
        }
        children.push_back(separatorEmpty());
        children.push_back(userTopArtistsTitle->Render());
        children.push_back(separator());
        if (spotifyData->topArtists.read().value.names.size() == 0) {
            queue.enqueue(SPOCLI::Event::GetUserTopArtists);
        }
        return vbox(children);
    });

    Component userImage = image_view(spotifyData);

    auto user_home = Container::Vertical({ Container::Horizontal({ user_Profile, userImage }), userTopTracksMenu });


    // auto user_home = Container::Vertical({ user_Profile, userTopTracksMenu });

    std::vector<std::string> tab_values{
        "Home",
        "User",
    };
    int tab_drawn = 0;
    auto tab_toggle = Toggle(&tab_values, &tab_drawn);

    auto tab_container = Container::Tab({ menu, user_home }, &tab_drawn);

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

    auto renderer = Renderer(container, [&] {
        return vbox({
                tab_toggle->Render(),
                separator(),
                tab_container->Render(),
                maybe_help_component->Render() | center,
        });
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

    renderLogin(std::ref(queue), spotifyData);

    screen.Loop(renderer);
}
