module helper;

using namespace std::string_view_literals;

static constexpr auto BOLD = "\033[1m"sv;
static constexpr auto BOLD_UNDERLINE = "\033[1;4m"sv;
static constexpr auto BOLD_RED = "\033[1;31m"sv;
static constexpr auto BOLD_GREEN = "\033[1;32m"sv;
static constexpr auto BOLD_YELLOW = "\033[1;33m"sv;
static constexpr auto BOLD_BRIGHT_BLACK = "\033[1;90m"sv;
static constexpr auto YELLOW = "\033[33m"sv;
static constexpr auto RESET = "\033[0m"sv;

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif

std::expected<void, std::error_code>
Helper::parse(const std::span<const std::string_view> args) {
    for (auto it = args.begin(); it != args.end(); ++it) {
        auto arg = *it;

        if (arg == "-h" || arg == "--help") {
            usage();
            help();
            return std::unexpected(
                    std::make_error_code(std::errc::operation_canceled));
        } else if (arg == "-v" || arg == "--version") {
            version();
            return std::unexpected(
                    std::make_error_code(std::errc::operation_canceled));
        } else {
            std::println("Unknown argument '{}{}{}'", YELLOW, arg, RESET);
            for_help();
            return std::unexpected(
                    std::make_error_code(std::errc::invalid_argument));
        }
    }

    return {};
}

void Helper::usage() {
    std::println("{0}Usage:{1} {2}spo-cli{1} [OPTIONS]", BOLD_UNDERLINE, RESET,
                 BOLD);
}

void Helper::help() {
    std::print("{0}Options:{1}\n"
               "  {2}-h, --help{1}              Show this help message\n"
               "  {2}-v, --version{1}           Show a version message\n",
               BOLD_UNDERLINE, RESET, BOLD);
}

void Helper::for_help() {
    std::println("For more information, try '{}--help{}'", BOLD, RESET);
}

void Helper::version() { std::println("spo-cli version: {}", GIT_VERSION); }
