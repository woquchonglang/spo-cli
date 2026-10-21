module;
export module event;

import std;
import spsc;
import mpmc;
import config;
import asio;
import spotify;
import ui;

export namespace SPOCLI {
enum class Event {
    Exit,
    Login,
    Raise,

    GetDevices,
    GetBrowseCategories,

    GetUserProfile,
    GetUserTopArtists,
    GetUserTopTracks,
    GetUserFollowedPlaylist,
    GetUserFollowedArtists,
    GetUserfollowArtistOrUsers,

    //plyaer
    GetCurrentlyPlayingTrack,
    GetAvailableDevice,
    GetUserQueue,

    GetUserPlaylists,

    GetUserSavedAlbums,
    GetUserSavedShows,
    GetUserSavedTracks,

    SkipToNext,
    SkipToPrevious,
    SeekRight,
    SeekLeft,
    SetVolumeUp,
    SetVolumeDown,

    Search,
    // lyrics
    GetLyrics,

    // playlists
    GetUserPlaylistItems,

    // soloist
    Play,
    Resume,
    Pause,
    Skip_next,
    Skip_prev,
    Set_volume,
    Set_shuffle,
    Set_repeat_context,
    Set_repeat_track,
    Add_to_queue,

    // ftxui
    Refresh,
    NextTrack
};
}

export moodycamel::Mpmc<SPOCLI::Event> eventQueue;
export moodycamel::Spsc<std::string> play_song_spsc;
export moodycamel::Spsc<std::string> playlistItems_spsc;
export moodycamel::Spsc<std::string> search_query;

export class AsyncEventHandler {
public:
    AsyncEventHandler(asio::io_context &ioc, SpotifyClient &client, Ui &ui);
    asio::awaitable<void> retry_get_devices();
    asio::awaitable<void> handle();

    void scheduleExitHandler(std::function<void()> handler) {
        exitHandler.push_back(handler);
    }

private:
    SpotifyClient &client;
    Ui &ui;
    asio::io_context &ioc;
    std::vector<std::function<void()>> exitHandler;
    asio::cancellation_signal cancel;
};
