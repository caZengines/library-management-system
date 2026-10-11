cmake_minimum_required(VERSION 3.20)

project(student_borrow
        VERSION 0.1.0
        DESCRIPTION "学生借书系统 —— 组合关系 + 依赖关系 (C++20)"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# 当前阶段：学生借书小系统（组合关系 + 依赖关系）
#   src/util/    —— 日期工具（借书日 / 应还日 / 逾期天数）
#   src/model/   —— 学生类 Student、图书类 Book
#   src/service/ —— 借阅管理者 BorrowManager（组合持有 + 依赖借书）
#   src/main.cpp —— 演示程序（7 个测试场景）
# 后续逐步扩充：持久化（文件读写） -> 界面层（菜单交互） -> 更多实体
# ---------------------------------------------------------------------------
set(UTIL_SOURCES
    src/util/date.cpp)

set(MODEL_SOURCES
    src/model/book.cpp
    src/model/student.cpp)

set(SERVICE_SOURCES
    src/service/borrow_manager.cpp)

add_executable(borrow_demo
    src/main.cpp
    ${UTIL_SOURCES}
    ${MODEL_SOURCES}
    ${SERVICE_SOURCES})

# 头文件按分区包含：src/model/book.h、src/service/borrow_manager.h
target_include_directories(borrow_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 源码统一使用 UTF-8 编码（含中文注释与提示文本）
if(MSVC)
    target_compile_options(borrow_demo PRIVATE /utf-8 /W4)
else()
    target_compile_options(borrow_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
