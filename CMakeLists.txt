cmake_minimum_required(VERSION 3.10)
project(lwcli)

# 设置工程根目录
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# 设置调试选项
set(CMAKE_BUILD_TYPE Debug)  # 设置为Debug模式
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")  # 添加-g调试符号

# 添加lwcli库
add_library(lwcli STATIC
    ${PROJECT_ROOT}/src/lwcli.c
    ${PROJECT_ROOT}/port/lwcli_port.c
)

# 添加头文件路径
target_include_directories(lwcli
    PUBLIC 
        ${PROJECT_ROOT}/inc
        ${PROJECT_ROOT}/port
)
