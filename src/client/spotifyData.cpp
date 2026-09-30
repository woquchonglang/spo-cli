module;
module spotify_data;

int SpotifyData::playback_progress() {
    auto guard = currentlyPlayTrack.read();
    if (!guard.value.isPlaying) {
        return guard.value.progress_ms;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() -
                           playback_last_updated_time)
                           .count();

    return guard.value.progress_ms + elapsed;
}

void SpotifyData::update_playback_last_updated_time() {
    playback_last_updated_time = std::chrono::steady_clock::now();
}
