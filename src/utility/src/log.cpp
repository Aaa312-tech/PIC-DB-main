#include "log.h"

// Define the global logger variable (no need to initialize it with nullptr)
std::shared_ptr<spdlog::logger> OPDB::logger;

void OPDB::initLogger()
{
  try {
    // Create file and console sinks
    auto              now        = std::chrono::system_clock::now();
    auto              now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timeInfo;
    timeInfo << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d-%H-%M-%S");
    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "logs/" + timeInfo.str() + ".txt", true);
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Combine sinks into a multi-sink logger
    std::vector<spdlog::sink_ptr> sinks = {fileSink, consoleSink};
    logger
        = std::make_shared<spdlog::logger>("OPDB", sinks.begin(), sinks.end());

    // Set log level
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);  // Set global log level to 'info'
    spdlog::flush_on(spdlog::level::info);   // Flush logs after each 'info' log
  } catch (const spdlog::spdlog_ex& ex) {
    std::cerr << "Log initialization failed: " << ex.what() << std::endl;
  }
}
