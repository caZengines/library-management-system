cmake_minimum_required(VERSION 3.20)

project(library_management
        VERSION 0.2.0
        DESCRIPTION "图书馆借阅管理系统 —— 继承/多态/模板/IO流 (C++20)"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# 分层结构：
#   src/util/        日期、字符串、控制台输入、表格打印（实验6 / 实验5）
#   src/model/       User/Student/Teacher、Book、BorrowRecord（实验3 / 实验4 / 实验6）
#   src/repository/  泛型仓储 Repository<T, KeyOf>（实验5，头文件实现）
#   src/service/     BorrowManager 业务核心（组合 + 依赖 + 多态）
#   src/ui/          菜单界面（实验6）
# ---------------------------------------------------------------------------
set(UTIL_SOURCES
    src/util/strings.cpp
    src/util/date.cpp
    src/util/io.cpp)

set(MODEL_SOURCES
    src/model/book.cpp
    src/model/user.cpp
    src/model/borrow_record.cpp)

set(SERVICE_SOURCES
    src/service/borrow_manager.cpp)

set(UI_SOURCES
    src/ui/menu.cpp)

add_executable(library_system
    src/main.cpp
    ${UTIL_SOURCES}
    ${MODEL_SOURCES}
    ${SERVICE_SOURCES}
    ${UI_SOURCES})

target_include_directories(library_system PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(library_system PRIVATE /utf-8 /W4)
else()
    target_compile_options(library_system PRIVATE -Wall -Wextra -Wpedantic)
endif()
