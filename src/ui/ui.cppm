module;
export module ui;

import ftxui;
import mpmc;
import std;
import spotify.data;
import config;
import asio;
import :theme;

using namespace ftxui;

export auto Event_ReConfig = Event::Special("reconfig");
export auto Event_NextTrack = Event::Special("nextTrack");

enum class TermDirection { Horizontal, Vertical };
enum class TermSize { Large, Medium, Small };
enum class NotifyLevel { Error, Info, Warning };

struct NotifyEntry {
    NotifyLevel level;
    std::string msg;
};

export class Ui {
public:
    Ui(config::Config &config, SpotifyData &spotifyData, asio::io_context &ioc);
    ~Ui();
    void reConfig();

    asio::awaitable<void> render();
    asio::awaitable<void> renderLogin();

    void updateFrame();

    const std::vector<::Image> &get_current_image();

    Element render_cava();
    void render_proc(bool enable);
    void render_help();
    void render_curlyrics();
    void render_username();
    void render_current_playing_song();
    void render_search();
    void render_image();
    void render_image_lyrics();
    void render_notify();

    void send_notify_msg(const std::string &msg);
    asio::awaitable<void> start_notify_get();

    App screen;

private:
    bool main_show = true;
    Component proc_section;
    Component help_section;
    bool help_show = false;
    Component curlyrics;
    Component user_name;
    Component current_playing_song;

    bool image_show = true;
    bool image_rendered = false;
    Component image_box;

    // search
    bool search_show = false;
    Component search_section;
    Component search_input;
    std::string search_name;

    Component notify_section;
    std::deque<Element> notify_msg;

    config::Config &config;

    // term
    Terminal::Quirks quirks;
    TermDirection term_direction;
    TermSize term_size = TermSize::Medium;
    TermSize last_term_size = TermSize::Medium;

    // image
    Component image_lycics_element;

    SpotifyData &spotifyData;

    std::string term_emulator_name;
    std::string term_emulator_version;

    ThemeManager theme_manager;

    // notify
    asio::experimental::channel<void(boost::system::error_code, std::string)>
            notify_msg_channel;
};
