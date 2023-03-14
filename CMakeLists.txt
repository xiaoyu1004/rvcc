cmake_minimum_required(VERSION 3.10)
project(rvcc)

option(ENBALE_TEST "enable test" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
file(GLOB_RECURSE ALL_SRCS "${PROJECT_SOURCE_DIR}/src/*.c" "${PROJECT_SOURCE_DIR}/src/*.h")
add_executable(${PROJECT_NAME} ${ALL_SRCS})

if(ENBALE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()