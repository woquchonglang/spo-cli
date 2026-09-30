module;
export module event;

import std;
import spotifyAuth;
import spotifyWebAPI;
import soloist;
import spsc;
import mpmc;
import librespot;

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
    GetUserQueue,

    GetUserPlaylists,

    GetUserSavedAlbums,
    GetUserSavedShows,
    GetUserSavedTracks,

    SeekRight,
    SeekLeft,

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


export class EventHandler {
public:
    EventHandler(SpotifyAuth &auth, SpotifyData& spotifyData,
                 Librespot &librespot)
            : api(nullptr, spotifyData), auth(auth), librespot(librespot) {};

    void handle(std::stop_token st);

    void scheduleExitHandler(std::function<void()> handler) {
        exitHandler.push_back(handler);
    }

private:
    SpotifyWebAPI api;
    SpotifyAuth auth;
    Librespot &librespot;

    std::vector<std::function<void()>> exitHandler;
};
