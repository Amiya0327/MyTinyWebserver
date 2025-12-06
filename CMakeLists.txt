cmake_minimum_required(VERSION 3.15)

project(Webserve)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(
    server
    main.cpp
    Webserver.cpp
    Timer.cpp
    HttpConn.cpp
    ./ThreadPool/ThreadPool.cpp
)