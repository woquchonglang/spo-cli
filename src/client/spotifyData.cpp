module;
module spotify.data;

import event;

int SpotifyData::playback_progress() {
    static bool query_update = false;

    auto guard = currentlyPlayTrack.read();
    if (!guard.value.isPlaying) {
        return guard.value.progress_ms;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() -
                           playback_last_updated_time)
                           .count();

    int total_progress = 0;
    int duration_ms = 0;
    {
        total_progress = guard.value.progress_ms + elapsed;
        duration_ms = guard.value.currently_playing.duration_ms;
    }

    if (total_progress < duration_ms) {
        query_update = false;
    } else if (total_progress == duration_ms && !query_update) {
        eventQueue.enqueue(SPOCLI::Event::GetCurrentlyPlayingTrack);
        query_update = true;
    } else if (total_progress > duration_ms) {
        total_progress = duration_ms;
    }
    return total_progress;
}

void SpotifyData::update_playback_last_updated_time() {
    playback_last_updated_time = std::chrono::steady_clock::now();
}
