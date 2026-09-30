module;
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
module log;

Log::Log() {
    spdlog::init_thread_pool(8192, 1);
    logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger",
                                                            "log", true);
    logger->set_pattern("[%H:%M:%S] [%t] [%^%l%$] %v");
    logger->set_level(spdlog::level::debug);
    this->info("log init");
    logger->flush_on(spdlog::level::trace);
}

Log::~Log() {
    this->info("log deinit");
    spdlog::shutdown();
}
