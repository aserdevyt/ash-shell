# Sets the minimum version of CMake required.
cmake_minimum_required(VERSION 3.10)

# Defines the project name and language.
project(ash C)

# Sets the C standard to C99.
set(CMAKE_C_STANDARD 99)

# Adds the 'include' directory for header files.
include_directories(include)

# Explicitly list all source files for a more reliable build.
# This ensures that CMake always knows about every file without needing to re-scan.
set(ASH_SRC
    src/aliases.c
    src/ashrc.c
    src/builtins.c
    src/commands.c
    src/config.c
    src/main.c
    src/parser.c
    src/prompt.c
    src/vars.c
)

# Adds the executable with the specified source files.
add_executable(ash ${ASH_SRC})

# Links the readline library, which is necessary for interactive input.
target_link_libraries(ash readline)
