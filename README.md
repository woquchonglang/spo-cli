# spo-cli

a spotify tui app, less is more

need: Spotify Premium

# Feature
os: linux (relying heavily on various features of Linux,not support windows and macOS, btw, I use arch linux)

- [ ] vim key
- [ ] local music
- [ ] animations
- [ ] fullscreen
- [ ] quick start and lazy load
- [ ] mouse
- [ ] config
- [ ] lyrics with autoscroll
- [ ] scope
- [ ] image render
- [ ] desktop notification
- [ ] cross-platform media control
- [ ] home
- [ ] async and multi-thread
- [ ] custom themes

## login spotify
### api
[spotify-web-api](https://developer.spotify.com/documentation/web-api)

### oAuth
[Authorization Code Flow](https://developer.spotify.com/documentation/web-api/tutorials/code-flow)

client_id
login_redirect_uri

[Rate Limits](https://developer.spotify.com/documentation/web-api/concepts/rate-limits)

### access token


### lyrics

[Genius](https://genius.com)

## build

### requirements
- cmake >= 4.4
- gcc >= 16.2
- linux >= 6.1
- openssl
- liburing


```
touch default.toml
```


```sh
cmake -B build -G Ninja
ninja -C build
```


## Inspiring by
- [lrxed](https://github.com/LunaPresent/lrxed.git)
- [jellyfin-tui](https://github.com/dhonus/jellyfin-tui)
- [termusic](https://github.com/tramhao/termusic.git)
- [ytui-music.git](https://github.com/sudipghimire533/ytui-music.git)
- [spotify-player](https://github.com/aome510/spotify-player.git)
- [go-musicfox](https://github.com/go-musicfox/go-musicfox.git)



