// ===========================================================================
//  student.h  ——  学生类
// ===========================================================================
#pragma once

#include "util/date.h"

#include <string>
#include <vector>

namespace borrow {

class Book;  // 前向声明

class Student {
public:
    // 借阅条目：某本书 + 借书日 + 应还日（体现"该学生借了哪些书"）
    struct BorrowItem {
        Book* book{nullptr};   // 指向被借图书
        Date borrow_date{};    // 借书日期
        Date due_date{};       // 应还日期
    };

    static constexpr int kDefaultMaxBorrow = 5;      // 最多可借 5 本
    static constexpr int kBorrowDays = 30;           // 借期 30 天
    static constexpr double kFinePerDay = 0.20;      // 逾期罚款 0.20 元/天

    Student() = default;
    Student(std::string id, std::string name, std::string major,
            int max_borrow = kDefaultMaxBorrow)
        : id_(std::move(id)),
          name_(std::move(name)),
          major_(std::move(major)),
          max_borrow_(max_borrow) {}

    // ---------------------------- 基本信息 ----------------------------
    const std::string& id() const noexcept { return id_; }      
    const std::string& name() const noexcept { return name_; }   
    const std::string& major() const noexcept { return major_; }
    int max_borrow() const noexcept { return max_borrow_; }

    void set_name(std::string name) { name_ = std::move(name); }
    void set_major(std::string major) { major_ = std::move(major); }
    void set_max_borrow(int value) { max_borrow_ = value; }

    // ---------------------- 借阅状态 ----------------------
    const std::vector<BorrowItem>& borrowed() const noexcept { return borrowed_; }
    int borrowed_count() const noexcept { return static_cast<int>(borrowed_.size()); }
    // 是否还能再借（未达到最大借阅数量）
    bool can_borrow_more() const noexcept { return borrowed_count() < max_borrow_; }
    // 是否已借这本书（同一本书不允许重复借）
    bool has_borrowed(const std::string& isbn) const noexcept;
    // 累计已缴罚款（由归还结算写入）
    double total_fine() const noexcept { return total_fine_; }
    void add_fine(double amount) { total_fine_ += amount; }

    // ---------------------------- 状态变更 ----------------------------
    // 记录一次借阅
    void add_borrowed_item(Book* book, const Date& borrow_date, const Date& due_date);
    // 移除一条借阅记录，返回是否成功
    bool remove_borrowed_item(const std::string& isbn);
    // 按 ISBN 找到借阅条目，找不到返回 nullptr
    const BorrowItem* find_item(const std::string& isbn) const;

    std::string to_string() const;

private:
    std::string id_;                  // 学号
    std::string name_;                // 姓名
    std::string major_;               // 专业
    int max_borrow_{kDefaultMaxBorrow};
    std::vector<BorrowItem> borrowed_; 
    double total_fine_{0.0};            // 累计罚款
};

}  // namespace borrow
