// ===========================================================================
//  strings.h  ——  字符串工具（纯函数，不涉及控制台）
//  ---------------------------------------------------------------------------
//  本文件为流解析（std::stringstream / getline）提供
//  切分与去空白的基础函数，供文件读取与菜单输入复用。
// ===========================================================================
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace borrow {

// 去掉首尾空白（含 \r，兼容 Windows 文本文件）
[[nodiscard]] std::string trim(std::string_view text);

// 以分隔符切分（默认 |，与数据文件格式一致）
[[nodiscard]] std::vector<std::string> split(std::string_view text, char delimiter = '|');

// 忽略大小写判断 text 是否包含 keyword（模糊查询用）
[[nodiscard]] bool contains_ignore_case(std::string_view text, std::string_view keyword);

}  // namespace borrow
