module;
#include <sdbus-c++/sdbus-c++.h>
#include <boost/system/error_code.hpp>
export module mpris;

import std;
import spotify.data;
import spsc;
import asio;

export enum class MpricEvent { all_update, exit };

/*
 *https://github.com/Kistler-Group/sdbus-cpp/blob/master/docs/using-sdbus-c++.md#integrating-sdbus-c-into-your-project
 */
export class MprisServer : public std::enable_shared_from_this<MprisServer> {
public:
    MprisServer(asio::any_io_executor ex, const SpotifyData &_spotifyData);
    void start();
    void stop();
    ~MprisServer();

    void notifyChanged();
    void notifyPropertiesChanged(
            const std::map<std::string, sdbus::Variant> &changed);
    asio::awaitable<void> handle_notify();

protected:
    void createInterfaceMediaPlayer2();
    void createInterfaceMediaPlayer2Player();
    void createInterfaceMediaPlayer2TrackList();
    void createInterfaceMediaPlayer2Playlists();

public:
    asio::experimental::channel<void(boost::system::error_code, MpricEvent)>
            notify_channel;

private:
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IObject> object;
    const SpotifyData &spotifyData;
    asio::any_io_executor ex;
};
