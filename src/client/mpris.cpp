module;
#include <sdbus-c++/sdbus-c++.h>
module mpris;

import event;

MprisServer::MprisServer(const SpotifyData &_spotifyData)
        : spotifyData(_spotifyData) {
    sdbus::ServiceName serviceName{ "org.mpris.MediaPlayer2.spocli" };

    try {
        connection = sdbus::createSessionBusConnection(serviceName);
    } catch (const sdbus::Error &e) {
        std::cerr << "Failed: " << e.what() << "\n";
    }

    sdbus::ObjectPath objectPath{ "/org/mpris/MediaPlayer2" };

    try {
        object = sdbus::createObject(*connection, std::move(objectPath));
    } catch (const sdbus::Error &e) {
        std::cerr << "Failed: " << e.what() << "\n";
    }

    createInterfaceMediaPlayer2();
    createInterfaceMediaPlayer2Player();

    connection->enterEventLoopAsync();

    handle_thread =
            std::jthread([this](std::stop_token st) { handle_notify(); });
}

MprisServer::~MprisServer() {
    mpris_notify_spsc.enqueue(MpricEvent::exit);
    if (connection) {
        connection->leaveEventLoop();
    }
    if (handle_thread.joinable()) {
        handle_thread.join();
    }
}

void MprisServer::handle_notify() {
    while (1) {
        MpricEvent event;
        mpris_notify_spsc.wait_dequeue(event);
        switch (event) {
        case MpricEvent::all_update:
            notifyChanged();
            break;
        case MpricEvent::exit:
            return;
        }
    }
}

void MprisServer::notifyPropertiesChanged(
        const std::map<std::string, sdbus::Variant> &changed) {
    object->emitSignal("PropertiesChanged")
            .onInterface("org.freedesktop.DBus.Properties")
            .withArguments("org.mpris.MediaPlayer2.Player", changed,
                           std::vector<std::string>{});
}

void MprisServer::notifyChanged() {
    sdbus::InterfaceName MediaPlayer2{ "org.mpris.MediaPlayer2" };
    object->emitPropertiesChangedSignal(MediaPlayer2);
    sdbus::InterfaceName MediaPlayer2Player{ "org.mpris.MediaPlayer2.Player" };
    object->emitPropertiesChangedSignal(MediaPlayer2Player);
}

// https://specifications.freedesktop.org/mpris/latest/Media_Player.html
void MprisServer::createInterfaceMediaPlayer2() {
    sdbus::InterfaceName MediaPlayer2{ "org.mpris.MediaPlayer2" };
    object->addVTable(
                  // Methods
                  sdbus::registerMethod("Raise").implementedAs(
                          []() { eventQueue.enqueue(SPOCLI::Event::Raise); }),
                  sdbus::registerMethod("Quit").implementedAs([this]() {
                      eventQueue.enqueue(SPOCLI::Event::Exit);
                  }),

                  // Properties
                  // Read only
                  sdbus::registerProperty("CanQuit").withGetter([]() {
                      return true; // 是否支持 Quit，返回 bool
                  }),
                  // Read/Write（有 getter + setter）
                  sdbus::registerProperty("Fullscreen")
                          .withGetter([]() { return false; })
                          .withSetter([](bool fullscreen) {
                              // 设置全屏状态
                          }),
                  sdbus::registerProperty("CanSetFullscreen").withGetter([]() {
                      return false;
                  }),

                  sdbus::registerProperty("CanRaise").withGetter([]() {
                      return false; // 是否支持 Raise，返回 bool
                  }),
                  sdbus::registerProperty("HasTrackList").withGetter([]() {
                      return false;
                  }),
                  sdbus::registerProperty("Identity").withGetter([]() {
                      return std::string("spo-cli");
                  }),
                  sdbus::registerProperty("DesktopEntry").withGetter([]() {
                      return std::string("spo-cli");
                  }),
                  sdbus::registerProperty("SupportedUriSchemes").withGetter([]() {
                      return std::vector<std::string>{
                          "file", "http"
                      }; // 支持的 URI scheme
                  }),
                  sdbus::registerProperty("SupportedMimeTypes").withGetter([]() {
                      return std::vector<std::string>{ "audio/mpeg",
                                                       "audio/flac" };
                  }))
            .forInterface("org.mpris.MediaPlayer2");
}

// https://specifications.freedesktop.org/mpris/latest/Player_Interface.html
void MprisServer::createInterfaceMediaPlayer2Player() {
    sdbus::InterfaceName MediaPlayer2Player{ "org.mpris.MediaPlayer2.Player" };

    object->addVTable(
                  sdbus::MethodVTableItem{
                          sdbus::MethodName{ "Next" },
                          sdbus::Signature{ "" },
                          {},
                          sdbus::Signature{ "" },
                          {},
                          [this](sdbus::MethodCall call) {
                              eventQueue.enqueue(SPOCLI::Event::SkipToNext);
                              auto reply = call.createReply();
                              reply.send();
                          },
                          {} },
                  sdbus::MethodVTableItem{
                          sdbus::MethodName{ "Previous" },
                          sdbus::Signature{ "" },
                          {},
                          sdbus::Signature{ "" },
                          {},
                          [this](sdbus::MethodCall call) {
                              eventQueue.enqueue(SPOCLI::Event::SkipToPrevious);
                              auto reply = call.createReply();
                              reply.send();
                          },
                          {} },
                  sdbus::MethodVTableItem{ sdbus::MethodName{ "Pause" },
                                           sdbus::Signature{ "" },
                                           {},
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               eventQueue.enqueue(
                                                       SPOCLI::Event::Pause);
                                           },
                                           {} },
                  sdbus::MethodVTableItem{ sdbus::MethodName{ "PlayPause" },
                                           sdbus::Signature{ "" },
                                           {},
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               eventQueue.enqueue(
                                                       SPOCLI::Event::Resume);
                                               auto reply = call.createReply();
                                               reply.send();
                                           },
                                           {} },

                  sdbus::MethodVTableItem{ sdbus::MethodName{ "Stop" },
                                           sdbus::Signature{ "" },
                                           {},
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               eventQueue.enqueue(
                                                       SPOCLI::Event::Pause);
                                           },
                                           {} },
                  sdbus::MethodVTableItem{ sdbus::MethodName{ "Play" },
                                           sdbus::Signature{ "" },
                                           {},
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               eventQueue.enqueue(
                                                       SPOCLI::Event::Play);
                                               auto reply = call.createReply();
                                               reply.send();
                                           },
                                           {} },
                  sdbus::MethodVTableItem{ sdbus::MethodName{ "Seek" },
                                           sdbus::Signature{ "x" },
                                           { "Offset" },
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               // eventQueue.enqueue(
                                               //         SPOCLI::Event::SeekRight); // TODO: add offset
                                               auto reply = call.createReply();
                                               reply.send();
                                           },
                                           {} },
                  sdbus::MethodVTableItem{ sdbus::MethodName{ "SetPosition" },
                                           sdbus::Signature{ "ox" },
                                           { "TrackId", "Position" },
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               // TODO: add set Position
                                               auto reply = call.createReply();
                                               reply.send();
                                           },
                                           {} },
                  sdbus::MethodVTableItem{ sdbus::MethodName{ "OpenUri" },
                                           sdbus::Signature{ "s" },
                                           { "Uri" },
                                           sdbus::Signature{ "" },
                                           {},
                                           [this](sdbus::MethodCall call) {
                                               // TODO: add openuri
                                               auto reply = call.createReply();
                                               reply.send();
                                           },
                                           {} },
                  // Signals
                  sdbus::registerSignal("Seeked").withParameters<int64_t>(),
                  // properties
                  sdbus::registerProperty("PlaybackStatus").withGetter([this]() {
                      auto guard = spotifyData.currentlyPlayTrack.read();
                      if (guard.value.isPlaying == true) {
                          return "Playing";
                      } else {
                          return "Paused";
                      }
                  }),
                  sdbus::registerProperty("LoopStatus").withGetter([this]() {
                      return "None";
                  }),
                  sdbus::registerProperty("Rate").withGetter(
                          [this]() { return 1.0; }),
                  sdbus::registerProperty("Shuffle").withGetter(
                          [this]() { return false; }),
                  sdbus::registerProperty("Metadata").withGetter([this]() {
                      std::map<std::string, sdbus::Variant> metadata;

                      auto guard = spotifyData.currentlyPlayTrack.read();

                      if (guard.value.currently_playing.name == "") {
                          return metadata; // 无曲目 → 空字典
                      }

                      // ★ 必须：mpris:trackid，类型 o（ObjectPath）
                      metadata["mpris:trackid"] = sdbus::Variant{
                          sdbus::ObjectPath{ "/org/spocli/controller/Track/" +
                                             guard.value.currently_playing.id }
                      };
                      //
                      // // 可选：长度（微秒）
                      metadata["mpris:length"] = sdbus::Variant{
                          static_cast<int64_t>(
                                  guard.value.currently_playing.duration_ms *
                                  1000)
                      };

                      // // 可选：标题
                      metadata["xesam:title"] = sdbus::Variant{
                          guard.value.currently_playing.name
                      };
                      //
                      // // 可选：艺术家列表（注意是 as，vector<string>）
                      metadata["xesam:artist"] = sdbus::Variant{
                          guard.value.currently_playing.atrists_name
                      };

                      // // 可选：专辑
                      // metadata["xesam:album"] = sdbus::Variant{
                      //     guard.value.currently_playing.atrists_name
                      // };
                      //
                      // // 可选：封面 URL
                      // if (!track->art_url.empty()) {
                      //     metadata["mpris:artUrl"] = sdbus::Variant{ track->art_url };
                      // }
                      //
                      return metadata;
                  }),
                  sdbus::registerProperty("Volume").withGetter(
                          [this]() { return 1.0; }),
                  sdbus::registerProperty("Position")
                          .withGetter([this]() -> int64_t {
                              auto guard =
                                      spotifyData.currentlyPlayTrack.read();
                              return guard.value.progress_ms * 1000;
                          }),
                  sdbus::registerProperty("MinimumRate").withGetter([this]() {
                      return 1.0;
                  }),
                  sdbus::registerProperty("MaximumRate").withGetter([this]() {
                      return 1.0;
                  }),
                  sdbus::registerProperty("CanGoNext").withGetter([]() {
                      return false;
                  }),
                  sdbus::registerProperty("CanGoPrevious").withGetter([]() {
                      return false;
                  }),
                  sdbus::registerProperty("CanPlay").withGetter([]() {
                      return true; // 是否支持 Raise，返回 bool
                  }),
                  sdbus::registerProperty("CanPause").withGetter([]() {
                      return true;
                  }),
                  sdbus::registerProperty("CanSeek").withGetter(
                          []() { return false; }),
                  sdbus::registerProperty("CanControl").withGetter([]() {
                      return true;
                  }))
            .forInterface(MediaPlayer2Player);
}
