// ===========================================================================
//  date.h  ——  简易日期工具（借书日 / 应还日 / 逾期天数 计算）
// ===========================================================================
#pragma once

#include <string>
#include <string_view>

namespace borrow {

class Date {
public:
    constexpr Date() = default;
    constexpr Date(int year, int month, int day) noexcept
        : year_(year), month_(month), day_(day) {}

    // 解析 "YYYY-MM-DD"，格式非法返回 false
    static bool parse(std::string_view text, Date& out);
    // 取系统当天日期
    static Date today();

    int year() const noexcept { return year_; }
    int month() const noexcept { return month_; }
    int day() const noexcept { return day_; }

    bool valid() const noexcept;
    std::string to_string() const;

    // 与 1970-01-01 相差的天数：两个日期相减即得天数差
    long long to_days() const noexcept;
    static Date from_days(long long days) noexcept;

    // 日期加减：借书日 + 借期 = 应还日
    Date add_days(long long days) const noexcept;
    // 相差天数（this -> other）
    long long days_until(const Date& other) const noexcept;

    friend bool operator==(const Date& a, const Date& b) noexcept {
        return a.to_days() == b.to_days();
    }
    friend bool operator<(const Date& a, const Date& b) noexcept {
        return a.to_days() < b.to_days();
    }
    friend bool operator>(const Date& a, const Date& b) noexcept { return b < a; }

private:
    int year_{1970};
    int month_{1};
    int day_{1};
};

}  // namespace borrow
