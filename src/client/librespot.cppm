module;
#include <boost/asio.hpp>
export module librespot;

import std;
import spotify_data;

namespace net = boost::asio;

export class Librespot {
public:
    Librespot(net::any_io_executor exec, SpotifyData &spotifyData);
    ~Librespot();
    void update_lyrics();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    SpotifyData& data;
    net::any_io_executor exec;
};
