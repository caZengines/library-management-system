// ===========================================================================
//  date.cpp  ——  日期计算实现（闰年判断 + 天数互转 + 逾期天数）
// ===========================================================================
#include "date.h"

#include <array>
#include <ctime>
#include <format>
#include <stdexcept>

namespace borrow {
namespace {

bool is_leap(int y) noexcept { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

int days_of_month(int y, int m) noexcept {
    constexpr std::array<int, 12> table{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && is_leap(y)) return 29;
    return table[static_cast<std::size_t>(m - 1)];
}

// 年月日 -> 距 1970-01-01 的天数
long long civil_to_days(int y, unsigned m, unsigned d) noexcept {
    long long yy = y;
    yy -= (m <= 2);
    const long long era = (yy >= 0 ? yy : yy - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(yy - era * 400);
    const unsigned doy = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u;
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097LL + static_cast<long long>(doe) - 719468LL;
}

// 天数 -> 年月日
void days_to_civil(long long z, int& y, int& m, int& d) noexcept {
    z += 719468LL;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    const int year = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    const unsigned mp = (5u * doy + 2u) / 153u;
    d = static_cast<int>(doy - (153u * mp + 2u) / 5u + 1u);
    m = static_cast<int>(mp + (mp < 10 ? 3u : -9u));
    y = year + (m <= 2 ? 1 : 0);
}

}  // namespace

bool Date::valid() const noexcept {
    if (year_ < 1900 || year_ > 2999) return false;
    if (month_ < 1 || month_ > 12) return false;
    return day_ >= 1 && day_ <= days_of_month(year_, month_);
}

bool Date::parse(std::string_view text, Date& out) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
    auto num = [&](std::size_t pos, int len) -> int {
        int value = 0;
        for (std::size_t i = pos; i < pos + static_cast<std::size_t>(len); ++i) {
            if (text[i] < '0' || text[i] > '9') return -1;
            value = value * 10 + (text[i] - '0');
        }
        return value;
    };
    const int y = num(0, 4);
    const int m = num(5, 2);
    const int d = num(8, 2);
    if (y < 0 || m < 0 || d < 0) return false;
    Date parsed{y, m, d};
    if (!parsed.valid()) return false;
    out = parsed;
    return true;
}

Date Date::today() {
    // 复用天数互转，避免依赖 <chrono> 日历部分
    const std::time_t now = std::time(nullptr);
    const long long days = static_cast<long long>(now / 86400);
    return from_days(days);
}

std::string Date::to_string() const {
    return std::format("{:04d}-{:02d}-{:02d}", year_, month_, day_);
}

long long Date::to_days() const noexcept {
    return civil_to_days(year_, static_cast<unsigned>(month_), static_cast<unsigned>(day_));
}

Date Date::from_days(long long days) noexcept {
    int y = 1970;
    int m = 1;
    int d = 1;
    days_to_civil(days, y, m, d);
    return Date{y, m, d};
}

Date Date::add_days(long long days) const noexcept { return from_days(to_days() + days); }

long long Date::days_until(const Date& other) const noexcept {
    return other.to_days() - to_days();
}

}  // namespace borrow
