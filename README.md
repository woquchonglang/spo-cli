# spo-cli

a spotify tui app, less is more

need: Spotify Premium

https://github.com/user-attachments/assets/639c608b-d8f3-4352-a462-c8b429f9353f

# Feature
os: linux (relying heavily on various features of Linux,not support windows and macOS, btw, I use arch linux)

- [ ] Simplicity
- [ ] Lightweight
- [ ] Barrier-free
- [ ] Quick start and lazy load
- [ ] Vim key, mouse
- [ ] Animations
- [ ] Hot reload config
- [ ] Lyrics with autoscroll
- [ ] Image render
- [ ] Desktop notification
- [ ] Async and coroutine
- [ ] Custom themes
- [ ] Adaptive screen

<img src=".assets/async.webp" width="200">

# Install

```sh 
yay -S spo-cli
```

or
```sh 
yay -S spo-cli-bin
```

## Login spotify
### api
[spotify-web-api](https://developer.spotify.com/documentation/web-api)

### oAuth
[Authorization Code PKCE Flow](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow)

client_id
login_redirect_uri

[Rate Limits](https://developer.spotify.com/documentation/web-api/concepts/rate-limits)

### access token


### lyrics

[Genius](https://genius.com)

## build

### requirements
- cmake >= 4.4
- clang >= 23
- linux >= 6.1
- openssl
- liburing


```
touch default.toml
```


```sh
cmake -B build -G Ninja
ninja -C build
./build/spo-cli
```

### Test
```cmake
option(SPOCLI_TEST ON)
```

```sh
./build/spo-test
```

### Install

```
cmake --install build --component spo-cli --prefix ~/.local
```


## Inspiring by
- [lrxed](https://github.com/LunaPresent/lrxed.git)
- [jellyfin-tui](https://github.com/dhonus/jellyfin-tui)
- [termusic](https://github.com/tramhao/termusic.git)
- [ytui-music.git](https://github.com/sudipghimire533/ytui-music.git)
- [spotify-player](https://github.com/aome510/spotify-player.git)
- [go-musicfox](https://github.com/go-musicfox/go-musicfox.git)
- [kew](https://codeberg.org/ravachol/kew?ref=terminaltrove)
- [myx](https://github.com/HaseebKhalid1507/Myx?ref=terminaltrove)


