module;
export module time;

import std;

export namespace Time {

std::string format_duration(int duration_ms) {
    const auto total_seconds = duration_ms / 1000;
    const auto minutes = total_seconds / 60;
    const auto seconds = total_seconds % 60;
    return std::format("{}:{:02}", minutes, seconds);
}

}
