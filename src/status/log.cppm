module;
#include <spdlog/spdlog.h>
export module log;

import singleton;

export class Log : public Singleton<Log> {
public:
    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args &&...args) {
        logger->info(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args &&...args) {
        logger->debug(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args &&...args) {
        logger->warn(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args &&...args) {
        logger->error(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void trace(std::format_string<Args...> fmt, Args &&...args) {
        logger->trace(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void critical(std::format_string<Args...> fmt, Args &&...args) {
        logger->critical(std::format(fmt, std::forward<Args>(args)...));
    }

private:
    Log();
    ~Log();
    friend class Singleton<Log>;

    std::shared_ptr<spdlog::logger> logger;
};
