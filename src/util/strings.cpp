// ===========================================================================
//  strings.cpp  ——  字符串工具实现
// ===========================================================================
#include "strings.h"

#include <algorithm>
#include <cctype>

namespace borrow {

std::string trim(std::string_view text) {
    const auto not_space = [](unsigned char c) { return !std::isspace(c); };
    auto begin = std::find_if(text.begin(), text.end(), not_space);
    auto end = std::find_if(text.rbegin(), text.rend(), not_space).base();
    if (begin >= end) return {};
    return std::string(begin, end);
}

std::vector<std::string> split(std::string_view text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char ch : text) {
        if (ch == delimiter) {
            parts.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(std::move(current));
    return parts;
}

bool contains_ignore_case(std::string_view text, std::string_view keyword) {
    auto lower = [](std::string_view value) {
        std::string out(value);
        std::ranges::transform(out, out.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return out;
    };
    return lower(text).find(lower(keyword)) != std::string::npos;
}

}  // namespace borrow
