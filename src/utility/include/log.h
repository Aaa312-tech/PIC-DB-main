#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

namespace OPDB {

// Global logger pointer
extern std::shared_ptr<spdlog::logger> logger;

// Initialize logger: set log level, add sinks, etc.
void initLogger();  // Declaration only, no inline definition here

}  // namespace OPDB
