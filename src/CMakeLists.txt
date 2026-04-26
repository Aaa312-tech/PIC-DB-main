add_subdirectory(utility)
add_subdirectory(db)
add_subdirectory(io)
add_subdirectory(algorithm)


add_executable(pr_lidar main.cpp)

set_target_properties(pr_lidar PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
find_package(ortools CONFIG REQUIRED)
set_property(TARGET pr_lidar PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(pr_lidar PRIVATE
  picpr_io
  picpr_db
  picpr_utility
  picpr_algo
  yaml-cpp::yaml-cpp
  spdlog::spdlog
  Boost::boost
  ortools::ortools
  ${GRAPHVIZ_LIBRARIES}
)