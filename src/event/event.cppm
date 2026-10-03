module;
#include <boost/asio.hpp>
export module event;

import std;
import spotifyAuthPKCE;
import spotifyWebAPI;
import soloist;
import spsc;
import mpmc;
import librespot;
import config;

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

    // soloist
    Play,
    Resume,
    Pause,
    Skip_next,
    Skip_prev,
    Set_volume,
    Set_shuffle,
    Seek_forward,
    Seek_backward,
    Set_repeat_context,
    Set_repeat_track,
    Add_to_queue,

    // ftxui
    Refresh
};

}

export moodycamel::Mpmc<SPOCLI::Event> eventQueue;
export moodycamel::Spsc<std::string> play_song_spsc;

export class AsyncEventHandler {
public:
    AsyncEventHandler(boost::asio::io_context &ioc, Config &config,
                      SpotifyData &spotifyData, Librespot &librespot);

    boost::asio::awaitable<void> handle();

    void scheduleExitHandler(std::function<void()> handler) {
        exitHandler.push_back(handler);
    }

private:
    SpotifyAuthPKCE auth;
    SpotifyWebAPI api;
    Librespot &librespot;
    boost::asio::io_context &ioc;
    std::vector<std::function<void()>> exitHandler;
    boost::asio::cancellation_signal cancel;
};
