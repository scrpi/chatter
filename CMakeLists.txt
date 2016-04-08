cmake_minimum_required (VERSION 2.8)
project (chatter)

set (CHATTER_VERSION_MAJOR 0)
set (CHATTER_VERSION_MINOR 1)

# Add chatter include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -pthread -std=c++1y")

# Add subdirectories
add_subdirectory (src)
add_subdirectory (examples)
