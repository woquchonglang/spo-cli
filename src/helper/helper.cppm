module;
export module helper;

import std;

export class Helper {
public:
    std::expected<void, std::error_code>
    parse(const std::span<const std::string_view> args);
    void usage();
    void help();
    void for_help();
    void version();
};
