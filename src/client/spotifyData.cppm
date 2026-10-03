module;
export module spotify_data;

import std;
import rwlock;

export {
    struct Image {
        std::string url;
        int height = 0;
        int width = 0;
    };

    struct SizesImages {
        std::vector<Image> largeImage;
        std::vector<Image> mediumImage;
        std::vector<Image> smallImage;
    };

    struct UserProfile {
        std::string account_id;
        std::string country;
        std::string displayName;
        std::string email;
        std::string product;
        Image image;
    };

    struct UserTopArtistsData {
        std::vector<std::string> names;
        std::vector<std::string> uri;
        SizesImages image;
    };

    struct UserTopTracksData {
        std::vector<std::string> uri;
        std::vector<std::string> tracks;
        SizesImages image;
    };

    struct UserPlaylistsData {
        std::vector<std::string> items;
        int total;
    };

    // player
    // "device" : {
    //     "id": "string",
    //     "is_active": false,
    //     "is_private_session": false,
    //     "is_restricted": false,
    //     "name": "Kitchen speaker",
    //     "type": "computer",
    //     "volume_percent": 59,
    //     "supports_volume": false
    // },

    struct Device {
        std::string id;
        bool is_active;
        bool is_private_session;
        bool is_restricted;
        std::string name;
        std::string type;
        int volume_percent;
        bool supports_volume;
    };

    struct PlaybackState {
        std::string repeat_state;
        bool shuffle_state;
        int timestamp;
        int progress_ms;
        bool isPlaying;

        int duration_ms;
    };

    struct CurrentlyPlaying {
        std::string name;
        std::vector<std::string> atrists_name;
        std::string id;
        std::string uri;
        int duration_ms;
        SizesImages image;
    };

    struct StartPlaying {};
    struct PausePlaying {};

    using PlayTarget =
            std::variant<std::monostate, std::string, std::vector<std::string>>;

    struct CurrentlyPlayingTrack {
        // [[deprecated(
        //         "spotify can't response librespot device, change to use GetAvailableDevices")]]
        Device device;
        CurrentlyPlaying currently_playing;
        int progress_ms;
        bool isPlaying;
    };

    struct UserQueueData {
        CurrentlyPlaying currently_playing;
    };

    // tracks
    struct Tracks {
        int duration_ms;
    };

    // lyrics
    struct LyricLine {
        std::int64_t time_ms;
        std::string text;
    };

    struct SpotifyData {
        RwLock<UserProfile> userProfile;
        RwLock<UserTopTracksData> topTracks;
        RwLock<UserTopArtistsData> topArtists;
        RwLock<UserPlaylistsData> userPlaylists;
        RwLock<UserQueueData> userQueue;
        // player
        RwLock<CurrentlyPlayingTrack> currentlyPlayTrack;
        RwLock<std::vector<Device>> devices;
        // lyrics
        RwLock<std::vector<LyricLine>> curLyric;

        RwLock<bool> isLogined;

        SpotifyData() {}

        int playback_progress();
        void update_playback_last_updated_time();

    private:
        std::chrono::steady_clock::time_point playback_last_updated_time;
    };
}
