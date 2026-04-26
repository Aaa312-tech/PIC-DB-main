add_library(picpr_algo)

find_path(GRAPHVIZ_INCLUDE_DIR 
    NAMES graphviz/gvc.h
    PATHS $ENV{CONDA_PREFIX}/include
    NO_DEFAULT_PATH 
)

find_library(GRAPHVIZ_GVC_LIBRARY 
    NAMES gvc
    PATHS $ENV{CONDA_PREFIX}/lib
    NO_DEFAULT_PATH
)

find_library(GRAPHVIZ_CGRAPH_LIBRARY 
    NAMES cgraph
    PATHS $ENV{CONDA_PREFIX}/lib
    NO_DEFAULT_PATH
)

if (GRAPHVIZ_INCLUDE_DIR AND GRAPHVIZ_GVC_LIBRARY AND GRAPHVIZ_CGRAPH_LIBRARY)
    message(STATUS "Found Conda Graphviz Include: ${GRAPHVIZ_INCLUDE_DIR}")
    
    # 2. Define the GRAPHVIZ_LIBRARIES variable so it can be used below
    set(GRAPHVIZ_LIBRARIES ${GRAPHVIZ_GVC_LIBRARY} ${GRAPHVIZ_CGRAPH_LIBRARY})
else()
    message(FATAL_ERROR "Graphviz libraries not found in Conda environment!")
endif()
target_sources(picpr_algo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/fast_route.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ilg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/init_place.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/iter_place.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/min_crx.cpp
)

target_include_directories(picpr_algo PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${GRAPHVIZ_INCLUDE_DIR}
)

find_package(ortools CONFIG REQUIRED)

target_link_libraries(picpr_algo PRIVATE
  picpr_io
  picpr_db
  picpr_utility
  Boost::boost
  ortools::ortools
  ${GRAPHVIZ_LIBRARIES}
)