
cmake_minimum_required(VERSION 3.10)
project(ash C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

file(GLOB ASH_SRC
    "src/*.c"
)

add_executable(ash ${ASH_SRC})
target_link_libraries(ash readline)
