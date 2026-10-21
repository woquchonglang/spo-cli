module;
#include <sdbus-c++/sdbus-c++.h>
export module mpris;

import std;
import spotify.data;
import spsc;

export enum class MpricEvent { all_update, exit };

export moodycamel::Block_spsc<MpricEvent> mpris_notify_spsc;

/*
 *https://github.com/Kistler-Group/sdbus-cpp/blob/master/docs/using-sdbus-c++.md#integrating-sdbus-c-into-your-project
 */
export class MprisServer {
public:
    MprisServer(const SpotifyData &_spotifyData);
    ~MprisServer();

    void notifyChanged();
    void notifyPropertiesChanged(
            const std::map<std::string, sdbus::Variant> &changed);
    void handle_notify();

protected:
    void createInterfaceMediaPlayer2();
    void createInterfaceMediaPlayer2Player();
    void createInterfaceMediaPlayer2TrackList();
    void createInterfaceMediaPlayer2Playlists();

private:
    std::unique_ptr<sdbus::IConnection> connection;
    std::unique_ptr<sdbus::IObject> object;
    const SpotifyData &spotifyData;
    std::jthread handle_thread;
};
