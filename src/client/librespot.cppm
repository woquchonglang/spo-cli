module;
export module spotify:librespot;

import std;
import spotify.data;
import asio;

export class Librespot {
public:
    Librespot(asio::thread_pool &pool, SpotifyData &spotifyData);
    void start();
    ~Librespot();
    void update_lyrics();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    SpotifyData &data;
};
