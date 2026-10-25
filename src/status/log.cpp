module;
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
module log;

import std;
namespace fs = std::filesystem;

Log::Log() {
    spdlog::init_thread_pool(8192, 1);
    fs::path dir = fs::path(getenv("HOME")) / ".config" / "spo-cli" / "log";
    logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger",
                                                            dir, true);
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    logger->set_level(spdlog::level::debug);
    this->info("log init");
    logger->flush_on(spdlog::level::trace);
    spdlog::flush_every(std::chrono::seconds(2));
}

Log::~Log() {
    this->info("log deinit");
    spdlog::shutdown();
}
