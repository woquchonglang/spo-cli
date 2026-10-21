module;
export module spotify;

import std;
import asio;
import config;

import :AuthPKCE;
import :WebAPI;
import :cache;
import :librespot;
export import spotify.data;

export class SpotifyClient {
public:
    SpotifyClient(asio::io_context &ioc, const Config &config,
                  asio::thread_pool &pool);
    ~SpotifyClient() = default;

public:
    SpotifyData data;
    SpotifyWebAPI api;
    SpotifyCache cache;
    SpotifyAuthPKCE auth;
    Librespot librespot;

private:
    void initDataFromCache();
};
