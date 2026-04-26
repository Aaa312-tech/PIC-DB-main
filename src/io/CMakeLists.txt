# src/io/CMakeLists.txt

add_library(picpr_io)

target_sources(picpr_io PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/defyml.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lefyml.cpp
)

target_include_directories(picpr_io PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(picpr_io PUBLIC
  picpr_db
  picpr_utility
  yaml-cpp::yaml-cpp
)
