module;
#include <boost/asio/awaitable.hpp>
module ui;

import ftxui;
import mpmc;
import std;
import event;
import proc;
import time;
import log;
import :visualizer;
import :progressGuage;
import :cover;
import :kittyImageComponent;
import magic_enum;

Ui::Ui(config::Config &config, SpotifyData &spotifyData)
        : screen(App::FullscreenAlternateScreen())
        , config(config)
        , spotifyData(spotifyData) {
    quirks = Terminal::GetQuirks();
    term_emulator_name = screen.TerminalEmulatorName();
    term_emulator_version = screen.TerminalEmulatorVersion();
    g_visualizer = std::make_unique<Visualizer>();
    g_visualizer->register_cb();
    if (config.data.read()->preset == "follow") {
    }
    reConfig();
    render_help();
    render_curlyrics();
    render_username();
    render_current_playing_song();
    render_search();
    render_image();
    render_notify();
}

Ui::~Ui(){
    screen.Exit();
}

asio::awaitable<void> Ui::renderLogin() {
    Log::instance().info("renderLogin init");
    eventQueue.enqueue(SPOCLI::Event::Login);

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
            main_show = false;
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
    std::vector<std::string> main_entries = {
        "Top Artists",     "Top Tracks",  "Playlists",  "Followed Playlists",
        "FollowedArtists", "SavedAlbums", "SavedShows", "SavedTracks"
    };

    int main_selected = 0;

    // int userTopTracksSelected = 0;
    // MenuOption userTopTracksOption;
    // userTopTracksOption.on_enter = [&userTopTracksSelected, &spotifyData] {
    //     auto guard = spotifyData.topTracks.read();
    //     auto uri = guard.value.uri[userTopTracksSelected];
    //     play_song_spsc.enqueue(uri);
    // };
    //
    // auto userTopTracksMenu = [&]() -> Component {
    //     auto guard = spotifyData.topTracks.read();
    //     return Menu(&guard.value.tracks, &userTopTracksSelected,
    //                 userTopTracksOption);
    // }();

    // auto userTopTracksMenu =
    //         Renderer(userTopTracksMenuComponent, [&]() -> Element {
    //             auto guard = spotifyData.topTracks.read();
    //             if (guard.value.tracks.size() > 0) {
    //                 return userTopTracksMenuComponent->Render();
    //             } else {
    //                 eventQueue.enqueue(SPOCLI::Event::GetUserTopTracks);
    //                 return text("Loading...") | center;
    //             }
    //         });


    // std::vector<Component> sub_menus;
    // std::vector<Component> sub_sub_menus;
    // std::vector<int> sub_sub_selected;
    // std::vector<bool> sub_sub_loaded;
    //
    // MenuOption topTracks_opt;
    // topTracks_opt.on_enter = [&sub_selected, &spotifyData] {
    //     auto idx = sub_selected[1];
    //     auto guard = spotifyData.topTracks.read();
    //     auto uri = guard.value.uri[idx];
    //     play_song_spsc.enqueue(uri);
    // };
    //
    // MenuOption playlists_opt;
    // playlists_opt.on_enter = [&sub_selected, &spotifyData] {
    //     auto idx = sub_selected[2];
    //     auto guard = spotifyData.userPlaylists.read();
    //     auto item = guard.value.items[idx];
    //     playlistItems_spsc.enqueue(item);
    // };
    //
    // // top artists
    // sub_menus.push_back(
    //         Menu(&spotifyData.topArtists.read().value.names, &sub_selected[0]));
    //
    // // top tracks
    // sub_menus.push_back(Menu(&spotifyData.topTracks.read().value.tracks,
    //                          &sub_selected[1], topTracks_opt));
    //
    // // user playlist
    // sub_menus.push_back(Menu(&spotifyData.userPlaylists.read().value.items,
    //                          &sub_selected[2]));
    //
    // for (int i = 3; i < 8; ++i) {
    //     sub_menus.push_back(Menu(&sub_entries[i], &sub_selected[i]));
    // }
    //
    MenuOption main_opt;
    main_opt.on_enter = [&] {
        int idx = main_selected;
        switch (idx) {
        case 0:
            eventQueue.enqueue(SPOCLI::Event::GetUserTopArtists);
            break;
        case 1:
            eventQueue.enqueue(SPOCLI::Event::GetUserTopTracks);
            break;
        case 2:
            eventQueue.enqueue(SPOCLI::Event::GetUserPlaylists);
            break;
        }
    };
    auto main_menu = Menu(&main_entries, &main_selected, main_opt);

    // container |= CatchEvent([&](Event event) {
    //     if (event == Event::r || event == Event::Escape) {
    //         return true;
    //     }
    //     return false;
    // });

    auto search_and_user_name = user_name | Modal(search_section, &search_show);

    auto container = Container::Vertical(
            { search_and_user_name, Renderer([] { return separator(); }),
              main_menu | vscroll_indicator | frame | size(HEIGHT, EQUAL, 8) |
                      size(WIDTH, EQUAL, 40) });

    auto container_wrapper = container | Modal(help_section, &help_show);

    // main ui
    auto renderer = Renderer(container_wrapper, [&] {
        Element image_lycics_com;
        if (term_direction == TermDirection::Horizontal) {
            image_lycics_com =
                    hbox({ image_box->Render(),
                           separatorEmpty() | size(WIDTH, EQUAL, 6),
                           vbox({
                                   filler(),
                                   curlyrics->Render() | size(WIDTH, EQUAL, 40),
                                   filler(),
                           }) }) |
                    center;
        } else {
            image_lycics_com =
                    vbox({ image_box->Render(),
                           separatorEmpty() | size(HEIGHT, EQUAL, 6),
                           vbox({
                                   filler(),
                                   curlyrics->Render() | size(WIDTH, EQUAL, 40),
                                   filler(),
                           }) }) |
                    center;
        }
        return vbox({ notify_section->Render() | align_right, filler(),
                      // container_wrapper->Render() | center,
                      image_lycics_com, filler(),
                      current_playing_song->Render() | center,
                      hbox({ filler(), proc_section->Render() }) });
    });

    renderer |= CatchEvent([&](Event event) {
        if (event == Event::q || event == Event::CtrlC) {
            main_show = false;
            eventQueue.enqueue(SPOCLI::Event::Exit);
            return true;
        } else if (event == Event::Character('?')) {
            help_show = !help_show;
            return true;
        } else if (event == Event::Character(' ')) {
            auto guard2 = spotifyData.currentlyPlayTrack.read();
            if (guard2.value.isPlaying == true) {
                eventQueue.enqueue(SPOCLI::Event::Pause);
                return true;
            } else {
                eventQueue.enqueue(SPOCLI::Event::Play);
                return true;
            }
        } else if (event == Event::Character('/')) {
            search_show = !search_show;
            return true;
        } else if (event == Event::ArrowRight) {
            eventQueue.enqueue(SPOCLI::Event::SeekRight);
            return true;
        } else if (event == Event::ArrowLeft) {
            eventQueue.enqueue(SPOCLI::Event::SeekLeft);
            return true;
        } else if (event == Event::Character('[')) {
            eventQueue.enqueue(SPOCLI::Event::SkipToPrevious);
            return true;
        } else if (event == Event::Character(']')) {
            eventQueue.enqueue(SPOCLI::Event::SkipToNext);
            return true;
        } else if (event == Event::Character('-')) {
            eventQueue.enqueue(SPOCLI::Event::SetVolumeDown);
            return true;
        } else if (event == Event::Character('=')) {
            eventQueue.enqueue(SPOCLI::Event::SetVolumeUp);
            return true;
        } else if (event == Event::Character('s')) {
            eventQueue.enqueue(SPOCLI::Event::Set_shuffle);
            return true;
        } else if (event == Event::Character('R')) {
            eventQueue.enqueue(SPOCLI::Event::Set_repeat_track);
            return true;
        } else if (event == Event::Event::i) {
            image_show = !image_show;
            return true;
        } else if (event == Event_ReConfig) {
            reConfig();
            return true;
        } else if (event == Event_NextTrack) {
            image_rendered = false;
            return true;
        }

        if (search_show) {
            if (event.is_character() || event == Event::Backspace ||
                event == Event::Return || event == Event::ArrowLeft ||
                event == Event::ArrowRight || event == Event::Delete ||
                event == Event::Escape) {
                return search_input->OnEvent(event);
            }
            return false;
        }

        return false;
    });

    co_await renderLogin();


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
    Log::instance().info("ui render exit");
}

void Ui::updateFrame() {
    auto size = Terminal::Size();

    if (size.dimy * 2 >= size.dimx) {
        term_direction = TermDirection::Vertical;
        if (size.dimy > 40 && size.dimx > 80) {
            term_size = TermSize::Large;
        } else if (size.dimy < 46 && size.dimx < 50) {
            term_size = TermSize::Small;
        } else {
            term_size = TermSize::Medium;
        }
    } else {
        term_direction = TermDirection::Horizontal;
        if (size.dimy > 34 && size.dimx > 128) {
            term_size = TermSize::Large;
        } else if (size.dimy < 80 && size.dimx < 24) {
            term_size = TermSize::Small;
        } else {
            term_size = TermSize::Medium;
        }
    }
    // Log::instance().info("term dimy:{},dimx:{},size:{},dire:{}", size.dimy,
    //                      size.dimx, magic_enum::enum_name(term_size),
    //                      magic_enum::enum_name(term_direction));
}

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


void Ui::render_proc(bool enable) {
    if (enable) {
        proc_section = Renderer([&] {
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
    } else {
        proc_section = Renderer([] { return emptyElement(); });
    }
}

void Ui::render_help() {
    // help show
    static std::vector<std::pair<std::string, std::string>> help_map = {
        { "?", "show help windows" },
        { "q", "exit app" },
        { "r", "return" },
        { "<enter>", "pause/play music" },
        { "[", "select the last song" },
        { "]", "select the next song" },
        { "/", "search anything" },
        { "i", "show image in new windows" },
        { "s", "shuffle the song" },
        { "R", "repeatt the song" },
        { "r", "return last windows" },
        { "-", "set song's volume down" },
        { "=", "set song's volume up" },
        { "a", "add this song to current queue" },
    };


    static auto left_column = Renderer([&] {
        Elements children = {};
        children.push_back(
                underlined(hbox({ filler(), text("Key:"), filler() })));
        for (auto i : help_map) {
            children.push_back(hbox({ filler(), text(i.first), filler() }));
        }
        return vbox(children);
    });

    static auto right_column = Renderer([&] {
        Elements children = {};
        children.push_back(
                underlined(hbox({ text("Description:"), filler() })));
        for (auto i : help_map) {
            children.push_back(hbox({ text(i.second), filler() }));
        }
        return vbox(children);
    });

    help_section = Renderer([&] {
        return window(text("Help"), hbox({
                                            filler(),
                                            left_column->Render(),
                                            separatorEmpty(),
                                            right_column->Render(),
                                            filler(),
                                    }) | size(WIDTH, EQUAL, 60) |
                                            size(HEIGHT, EQUAL, 20));
    });
}

void Ui::render_curlyrics() {
    static std::string last_lyrics_request_id;
    curlyrics = Renderer([&]() -> Element {
        static int tick = 0;

        {
            auto guard1 = spotifyData.currentlyPlayTrack.read();
            if (guard1.value.currently_playing.name == "") {
                return emptyElement();
            }

            if (last_lyrics_request_id != guard1.value.currently_playing.id) {
                eventQueue.enqueue(SPOCLI::Event::GetLyrics);
                last_lyrics_request_id = guard1.value.currently_playing.id;
                tick = 0;
            }
        }

        std::vector<LyricLine> lyricLine;
        {
            lyricLine = spotifyData.curLyric.read().value;
            if (lyricLine.size() == 0) {
                return text("");
            }
        }

        int current = 0;
        const int offset = 300; //FIX: 300ms offset for lyrics display
        auto progress_ms = spotifyData.playback_progress() + offset;
        for (int i = 0; i < lyricLine.size(); i++) {
            if (lyricLine[i].time_ms <= progress_ms) {
                current = i;
            } else {
                break;
            }
        }

        int start = (current == 0) ? 0 : current - 1;
        int end = std::min(start + 3, (int)lyricLine.size());
        start = (end >= 3) ? end - 3 : 0;

        Elements rows;
        for (int i = start; i < end; i++) {
            auto style = (i == current) ?
                                 color(config.data.read()->theme.lyrics_fg) |
                                         bold :
                                 color(config.data.read()->theme.lyrics_bg);
            rows.push_back(text(lyricLine[i].text) | style);
        }

        return vbox(std::move(rows));
    });
}

void Ui::render_username() {
    user_name = Renderer([this]() -> Element {
        auto guard = spotifyData.userProfile.read();
        if (guard.value.displayName != "") {
            return text("Hi! " + guard.value.displayName);
        } else {
            eventQueue.enqueue(SPOCLI::Event::GetUserProfile);
            return text("Hi!");
        }
    });
}

void Ui::render_current_playing_song() {
    static bool request_currently_playing_track = false;
    current_playing_song = Renderer([this]() -> Element {
        auto guard2 = spotifyData.currentlyPlayTrack.read();

        if (guard2.value.currently_playing.duration_ms != 0) {
            std::string isPlaying = guard2.value.isPlaying ? "playing" : "stop";
            auto progress_ms = spotifyData.playback_progress();
            return vbox(
                    { render_cava() | flex,
                      text(isPlaying + ": " +
                           guard2.value.currently_playing.name + " " +
                           guard2.value.currently_playing.atrists_name[0] +
                           "[" + Time::format_duration(progress_ms) + "/" +
                           Time::format_duration(
                                   guard2.value.currently_playing.duration_ms) +
                           "]"),
                      hbox({
                              progressGaugeCharset(
                                      ((float)progress_ms /
                                       guard2.value.currently_playing
                                               .duration_ms),
                                      { "─", "━" },
                                      config.data.read()->theme.progressline_fg,
                                      config.data.read()->theme.progressline_bg) |
                                      size(WIDTH, EQUAL, 60),
                      }) });
        } else {
            if (request_currently_playing_track == false) {
                eventQueue.enqueue(SPOCLI::Event::GetCurrentlyPlayingTrack);
                request_currently_playing_track = true;
            }
            return vbox(
                    { text("no play"),
                      hbox({
                              progressGaugeCharset(
                                      0, { "—", "━" },
                                      config.data.read()->theme.progressline_fg,
                                      config.data.read()->theme.progressline_bg) |
                                      size(WIDTH, EQUAL, 60),
                      }) });
        }
    });
}

// TODO: add cur page search and remote search,now only support remote search
void Ui::render_search() {
    InputOption search_option;
    search_option.multiline = false;
    search_option.on_enter = [this]() { search_query.enqueue(search_name); };
    search_input = Input(&search_name, "", search_option);

    search_input |= CatchEvent([&](Event event) {
        if (event == Event::Escape) {
            search_name.clear();
            search_show = false;
            return true;
        }
        return false;
    });

    search_section = Renderer([&]() -> Element {
        return hbox(text(": "), search_input->Render());
    });
}

const std::vector<::Image> &Ui::get_current_image() {
    if (last_term_size != term_size) {
        image_rendered = false;
        last_term_size = term_size;
    }
    if (term_size == TermSize::Large) {
        return spotifyData.currentlyPlayTrack.read()
                .value.currently_playing.image.largeImage;
    } else if (term_size == TermSize::Medium) {
        return spotifyData.currentlyPlayTrack.read()
                .value.currently_playing.image.mediumImage;
    } else {
        return spotifyData.currentlyPlayTrack.read()
                .value.currently_playing.image.smallImage;
    }
}

void Ui::render_image() {
    static Component image_component_;
    image_box = Renderer([&]() -> Element {
        const std::vector<::Image> &image = get_current_image();
        if (image.empty()) {
            return emptyElement();
        } else if (!image_rendered) {
            Log::instance().info("image size: {}x{}", image[0].width,
                                 image[0].height);
            image_component_ = image_view(image[0]);
            image_rendered = true;

            //TODO: update theme if select following theme
            if (theme_manager.follow_picker) {
                theme_manager.update(image[0]);
                auto _data = *config.data.read();
                Color color = Color::RGBA(theme_manager.palette.colors[0].r,
                                          theme_manager.palette.colors[0].g,
                                          theme_manager.palette.colors[0].b,
                                          255);
                _data.theme.lyrics_fg = color;
                config.data.write(std::make_shared<config::Data>(_data));
            }
        }
        return image_component_->Render();
    });
}

void Ui::render_notify() {
    notify_section = Renderer([&]() -> Element {
        if (auto msg = config.notify_msg.read(); msg && !msg->empty()) {
            notify_msg.push_back(*msg);
        }
        if (!notify_msg.empty()) {
            auto left_bar = separatorLight() | color(Color::Red);
            auto content = text(notify_msg[0]) | flex;
            auto element = hbox({
                    left_bar,
                    content,
            });
            return element;
        }
        return emptyElement();
    });
}


void Ui::reConfig() { render_proc(config.data.read()->proc.enable); }
