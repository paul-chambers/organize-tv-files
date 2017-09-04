cmake_minimum_required(VERSION 3.6)
project(cpynewer)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)
add_executable(cpynewer ${SOURCE_FILES})
target_link_libraries(cpynewer popt)
