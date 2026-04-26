add_library(picpr_db)

target_sources(picpr_db PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cell.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/instance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/segment.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/segmentGraph.cpp
)

target_include_directories(picpr_db PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(picpr_db PUBLIC
  picpr_utility
  picpr_algo
  Boost::boost
)