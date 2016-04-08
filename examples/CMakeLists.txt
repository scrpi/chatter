
set (SRC_ROOT ${PROJECT_SOURCE_DIR}/examples)

set (SRC
    ${SRC_ROOT}/main.cpp
)

add_executable(chex ${SRC})

target_link_libraries(chex chatter)
