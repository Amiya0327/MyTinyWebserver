cmake_minimum_required(VERSION 3.15)

project(Webserve)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(
    server
    main.cpp
    Webserver.cpp
    ./Timer/Timer.cpp
    ./HttpConn/HttpConn.cpp
    ./ThreadPool/ThreadPool.cpp
    ./ConnPool/ConnPool.cpp
    ./ConnPool/MysqlConn.cpp
)

target_link_libraries(server PRIVATE mysqlclient)