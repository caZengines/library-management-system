// ===========================================================================
//  user.cpp  ——  用户体系实现（多态、借阅清单、文本序列化）
// ===========================================================================
#include "user.h"

#include "model/book.h"
#include "util/strings.h"

#include <algorithm>
#include <format>
#include <ostream>
#include <stdexcept>
#include <string>

namespace borrow {
namespace {

// 字段转 int / double（文件可能被手工改坏，解析失败返回 nullopt）
std::optional<int> to_int(const std::string& text) {
    try {
        std::size_t used = 0;
        const int value = std::stoi(text, &used);
        if (used != text.size()) return std::nullopt;
        return value;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

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

// ===========================================================================
//  User 公共实现
// ===========================================================================
User::User(std::string id, std::string name, std::string department)
    : id_(std::move(id)), name_(std::move(name)), department_(std::move(department)) {}

bool User::has_borrowed(const std::string& isbn) const noexcept {
    // 【实验5·模板使用】std::ranges 算法复用：对任意容器都能按条件判断
    return std::ranges::any_of(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
}

void User::add_borrowed_item(Book* book, const Date& borrow_date, const Date& due_date) {
    if (book == nullptr) throw std::invalid_argument("借阅记录中的图书不能为空");
    borrowed_.push_back(BorrowItem{book, borrow_date, due_date});
}

bool User::remove_borrowed_item(const std::string& isbn) {
    // 删除容器元素会让迭代器失效，这里用 C++20 的 erase_if 一次完成"筛选 + 截断"
    const auto removed = std::erase_if(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
    return removed > 0;
}

const User::BorrowItem* User::find_item(const std::string& isbn) const {
    const auto it = std::ranges::find_if(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
    return it == borrowed_.end() ? nullptr : &*it;
}

std::string User::to_string() const {
    return std::format("{:<10} {:<4} {:<10} {:<12} {:<16} {:>4} {:>4} {:>8.2f}", id_, role_name(),
                       name_, department_, extra_info(), max_borrow(), borrow_days(),
                       fine_per_day());
}

// 【实验4·多态性与虚函数】print() 内部调用虚函数 role_name()/extra_info()/
// max_borrow()/borrow_days()/fine_per_day()，同一个基类实现因对象真实类型不同
// 而输出不同内容——这就是运行时多态。
std::ostream& User::print(std::ostream& out) const {
    out << std::format("{}（{}）| 院系：{} | {} | 限借 {} 本 | 借期 {} 天 | 逾期 {:.2f} 元/天",
                       name_, role_name(), department_, extra_info(), max_borrow(), borrow_days(),
                       fine_per_day());
    if (borrowed_count() > 0) {
        out << std::format(" | 已借 {}/{} 本", borrowed_count(), max_borrow());
    }
    if (total_fine_ > 0.0) {
        out << std::format(" | 累计罚款 {:.2f} 元", total_fine_);
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const User& user) { return user.print(out); }

// 【实验6·输入输出流】文本 -> 对象 的工厂：
// 根据首字段区分 STUDENT / TEACHER，再调用对应派生类构造。
std::optional<std::unique_ptr<User>> User::deserialize(const std::string& line) {
    const auto fields = split(line, '|');
    if (fields.empty()) return std::nullopt;

    if (fields[0] == "STUDENT") {
        // STUDENT|学号|姓名|院系|专业|年级|最大借阅数量|累计罚款
        if (fields.size() != 8) return std::nullopt;
        const auto max_borrow = to_int(fields[6]);
        const auto fine = to_double(fields[7]);
        if (!max_borrow || !fine) return std::nullopt;
        auto student = std::make_unique<Student>(fields[1], fields[2], fields[3], fields[4],
                                                 fields[5]);
        student->set_max_borrow(*max_borrow);
        student->add_fine(*fine);
        return student;
    }
    if (fields[0] == "TEACHER") {
        // TEACHER|工号|姓名|院系|职称|最大借阅数量|累计罚款
        if (fields.size() != 7) return std::nullopt;
        const auto max_borrow = to_int(fields[5]);
        const auto fine = to_double(fields[6]);
        if (!max_borrow || !fine) return std::nullopt;
        auto teacher = std::make_unique<Teacher>(fields[1], fields[2], fields[3], fields[4]);
        teacher->set_max_borrow(*max_borrow);
        teacher->add_fine(*fine);
        return teacher;
    }
    return std::nullopt;  // 未知角色
}

// ===========================================================================
//  学生
// ===========================================================================
Student::Student(std::string id, std::string name, std::string department,
                 std::string major, std::string grade, int max_borrow)
    : User(std::move(id), std::move(name), std::move(department)),
      major_(std::move(major)),
      grade_(std::move(grade)),
      max_borrow_(max_borrow) {}

std::string Student::extra_info() const {
    if (major_.empty() && grade_.empty()) return "专业/年级未填写";
    return std::format("{}/{}", major_, grade_);
}

std::string Student::serialize() const {
    return std::format("STUDENT|{}|{}|{}|{}|{}|{}|{:.2f}", id_, name_, department_, major_, grade_,
                       max_borrow_, total_fine_);
}

// ===========================================================================
//  教师
// ===========================================================================
Teacher::Teacher(std::string id, std::string name, std::string department,
                 std::string title, int max_borrow)
    : User(std::move(id), std::move(name), std::move(department)),
      title_(std::move(title)),
      max_borrow_(max_borrow) {}

std::string Teacher::extra_info() const {
    return title_.empty() ? std::string{"职称未填写"} : title_;
}

std::string Teacher::serialize() const {
    return std::format("TEACHER|{}|{}|{}|{}|{}|{:.2f}", id_, name_, department_, title_,
                       max_borrow_, total_fine_);
}

}  // namespace borrow
