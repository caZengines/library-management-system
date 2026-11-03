// ===========================================================================
//  io.h  ——  控制台输入（带校验与重试）
//  ---------------------------------------------------------------------------
//   输入输出流本文件集中展示标准输入输出流的使用：
//       std::cin / std::cout、std::getline 读整行、std::from_chars 解析数字、
//       非法输入时的流状态清理与重试（clear / ignore）。
//   业务层与模型层不再直接使用 cin/cout，界面层通过本模块统一收集输入。
// ===========================================================================
#pragma once

#include "util/date.h"

#include <optional>
#include <string>

namespace borrow::io {

// 读一行（自动去掉首尾空白）；输入流结束返回空串
[[nodiscard]] std::string read_line(const std::string& prompt);

// 读一行，非空才返回；空行表示取消
[[nodiscard]] std::optional<std::string> read_required(const std::string& prompt);

// 读整数并限制范围，非法输入反复提示
[[nodiscard]] int read_int(const std::string& prompt, int min, int max);

// 读浮点数并限制范围
[[nodiscard]] double read_double(const std::string& prompt, double min, double max);

// 读日期（YYYY-MM-DD），非法输入反复提示
[[nodiscard]] Date read_date(const std::string& prompt);

// 读 [y/N] 确认
[[nodiscard]] bool read_yes_no(const std::string& prompt);

// 按任意键继续（用于菜单暂停）
void pause();

}  // namespace borrow::io
