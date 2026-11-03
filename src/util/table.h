// ===========================================================================
//  table.h  ——  表格打印
// ===========================================================================
#pragma once

#include <concepts>
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace borrow {

// 概念：可作为表格行的类型
template <typename T>
concept TableEntity = requires(const T& value) {
    { value.id() } -> std::convertible_to<const std::string&>;
};

// 函数模板：打印"指针列表"形式的表格
//   title   —— 表格标题
//   rows    —— 行对象指针集合
//   header  —— 表头字符串
//   row_fn  —— 把单个对象格式化为一行文本的可调用对象
template <TableEntity T, typename RowFn>
void print_table(std::ostream& out,
                 std::string_view title,
                 const std::vector<const T*>& rows,
                 std::string_view header,
                 RowFn row_fn) {
    constexpr std::string_view kSeparator =
        "--------------------------------------------------------------------------------";
    out << std::format("\n{}（共 {} 条）\n", title, rows.size());
    if (rows.empty()) {
        out << "  （暂无记录）\n";
        return;
    }
    out << kSeparator << '\n';
    out << "  " << header << '\n';
    out << kSeparator << '\n';
    for (const T* row : rows) {
        if (row != nullptr) out << "  " << row_fn(*row) << '\n';
    }
    out << kSeparator << '\n';
}

}  // namespace borrow
