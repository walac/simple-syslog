cmake_minimum_required(VERSION 3.16.0)

project(log-server)

set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(log-server src/main.c src/socket.c src/error.c)
target_compile_options(log-server PRIVATE -Wall -Wextra)
target_link_libraries(log-server)
