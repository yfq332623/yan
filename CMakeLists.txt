cmake_minimum_required(VERSION 3.10)
project(myweb CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件：src 目录下所有 .cpp
set(SOURCES
    src/main.cpp
    src/webServer.cpp
    src/connection_pool.cpp
    src/log.cpp
    src/util_timer.cpp
    src/http.cpp
)

# 可执行文件 server
add_executable(server ${SOURCES})

# 头文件搜索路径：include 目录，这样 #include "webServer.h" 等能找到
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 链接 pthread 和 mysqlclient（与原来 Makefile 一致）
target_link_libraries(server PRIVATE pthread mysqlclient)

# 可选：编译选项
target_compile_options(server PRIVATE -Wall -O2 -g)
