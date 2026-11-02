// ===========================================================================
//  borrow_record.cpp  ——  借阅记录实现（状态判定 / 逾期天数 / 序列化）
// ===========================================================================
#include "borrow_record.h"

#include "util/strings.h"

#include <format>
#include <istream>
#include <ostream>
#include <string>

namespace borrow {
namespace {

std::optional<double> to_double(const std::string& text) {
    try {
        std::size_t used = 0;
        const double value = std::stod(text, &used);
        if (used != text.size()) return std::nullopt;
        return value;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

RecordStatus BorrowRecord::status(const Date& today) const {
    if (returned_) return RecordStatus::Returned;
    return today > due_date_ ? RecordStatus::Overdue : RecordStatus::Borrowed;
}

std::string BorrowRecord::status_text(const Date& today) const {
    switch (status(today)) {
        case RecordStatus::Returned:
            return "已归还";
        case RecordStatus::Overdue:
            return std::format("逾期{}天", overdue_days_now(today));
        case RecordStatus::Borrowed:
        default:
            return "借阅中";
    }
}

long long BorrowRecord::overdue_days_now(const Date& today) const {
    if (returned_) return 0;
    const long long days = due_date_.days_until(today);
    return days > 0 ? days : 0;
}

std::string BorrowRecord::serialize() const {
    const std::string return_text = returned_ ? return_date_.to_string() : std::string{"-"};
    return std::format("{}|{}|{}|{}|{}|{}|{:.2f}|{}", id_, user_id_, book_isbn_,
                       borrow_date_.to_string(), due_date_.to_string(), return_text, fine_,
                       returned_ ? 1 : 0);
}

std::optional<BorrowRecord> BorrowRecord::parse(std::string_view line) {
    const auto fields = split(line, '|');
    if (fields.size() != 8) return std::nullopt;

    Date borrow_date;
    Date due_date;
    if (!Date::parse(fields[3], borrow_date) || !Date::parse(fields[4], due_date)) {
        return std::nullopt;
    }
    const auto fine = to_double(fields[6]);
    if (!fine) return std::nullopt;

    const bool returned = fields[7] == "1";
    Date return_date{};
    if (returned) {
        if (!Date::parse(fields[5], return_date)) return std::nullopt;
    }
    return BorrowRecord{fields[0], fields[1], fields[2], borrow_date, due_date, return_date,
                        *fine, returned};
}

std::string BorrowRecord::to_string() const {
    return std::format("{:<8} {:<10} {:<16} {:<12} {:<12} {:<12} {:>8.2f}", id_, user_id_,
                       book_isbn_, borrow_date_.to_string(), due_date_.to_string(),
                       returned_ ? return_date_.to_string() : std::string{"-"}, fine_);
}

// ===========================================================================
//  BorrowRecord 的流运算符重载
// ===========================================================================
std::ostream& operator<<(std::ostream& out, const BorrowRecord& record) {
    return out << std::format("记录 {} | 用户 {} | ISBN {} | 应还 {} | {} | {:.2f} 元", record.id_,
                              record.user_id_, record.book_isbn_, record.due_date_.to_string(),
                              record.returned_ ? "已归还" : "在借", record.fine_);
}

std::istream& operator>>(std::istream& in, BorrowRecord& record) {
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        if (auto parsed = BorrowRecord::parse(line)) {
            record = std::move(*parsed);
            return in;
        }
        // 损坏行：跳过并继续
    }
    in.setstate(std::ios::failbit);
    return in;
}

}  // namespace borrow
