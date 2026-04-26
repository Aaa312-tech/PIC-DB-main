add_library(picpr_utility)

target_sources(picpr_utility PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/log.cpp
)

target_include_directories(picpr_utility PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(picpr_utility PUBLIC
  spdlog::spdlog
  yaml-cpp::yaml-cpp
)