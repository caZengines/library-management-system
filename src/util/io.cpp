// ===========================================================================
//  io.cpp  ——  控制台输入实现
// ===========================================================================
#include "io.h"

#include "util/strings.h"

#include <charconv>
#include <cstdio>
#include <iostream>
#include <string>

namespace borrow::io {

// std::getline 读取一整行（允许输入中包含空格），
// 再用 trim 去掉首尾空白。遇到 EOF 时清除流状态，返回空串。
std::string read_line(const std::string& prompt) {
    if (!prompt.empty()) std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return {};
    }
    return trim(line);
}

std::optional<std::string> read_required(const std::string& prompt) {
    const std::string value = read_line(prompt);
    if (value.empty()) return std::nullopt;
    return value;
}

// 数字输入：
//   - std::getline 先取一行；
//   - std::from_chars 做严格整数解析
//   - 非法输入时打印提示并重新读取，避免 std::cin 处于错误状态。
int read_int(const std::string& prompt, int min, int max) {
    for (;;) {
        const std::string text = read_line(prompt);
        if (text.empty()) return min;  // EOF/空输入：返回下界，避免死循环
        int value = 0;
        const auto* first = text.data();
        const auto* last = first + text.size();
        if (auto [ptr, ec] = std::from_chars(first, last, value);
            ec == std::errc{} && ptr == last && value >= min && value <= max) {
            return value;
        }
        std::cout << "  [tips] 请输入 " << min << " ~ " << max << " 之间的整数。\n";
    }
}

double read_double(const std::string& prompt, double min, double max) {
    for (;;) {
        const std::string text = read_line(prompt);
        if (text.empty()) return min;
        try {
            std::size_t used = 0;
            const double value = std::stod(text, &used);
            if (used == text.size() && value >= min && value <= max) return value;
        } catch (const std::exception&) {
            // 解析失败，落到下面的提示
        }
        std::cout << "  [tips] 请输入 " << min << " ~ " << max << " 之间的数字。\n";
    }
}

// 日期输入：复用 Date::parse，失败后循环提示
Date read_date(const std::string& prompt) {
    for (;;) {
        const std::string text = read_line(prompt);
        Date date;
        if (Date::parse(text, date)) return date;
        std::cout << "  [tips] 日期格式应为 YYYY-MM-DD（例如 2026-10-29），请重新输入。\n";
    }
}

bool read_yes_no(const std::string& prompt) {
    const std::string answer = read_line(prompt + " [y/N]: ");
    if (answer.empty()) return false;
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES";
}

void pause() {
    std::cout << "\n按回车键继续...";
    std::cout.flush();
    std::cin.get();          // 等待一次回车
    if (!std::cin) std::cin.clear();
}

}  // namespace borrow::io
