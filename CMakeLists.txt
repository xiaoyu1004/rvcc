cmake_minimum_required(VERSION 3.10)
project(rvcc)

option(ENBALE_TEST "enable test" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
set(CMAKE_C_STANDARD 11)
set(srcs src/main.c)
add_executable(${PROJECT_NAME} ${srcs})

if(ENBALE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()