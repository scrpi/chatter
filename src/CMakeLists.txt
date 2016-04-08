cmake_minimum_required(VERSION 2.8)

set (LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

set (SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

set (SRC
    ${SRC_ROOT}/host.cpp
    ${SRC_ROOT}/hostaddress.cpp
    ${SRC_ROOT}/packet.cpp
    ${SRC_ROOT}/packet_listener.cpp
    ${SRC_ROOT}/peer.cpp
    ${SRC_ROOT}/protocol.cpp
    ${SRC_ROOT}/unix.cpp
)

add_library(chatter STATIC ${SRC})
