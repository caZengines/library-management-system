// ===========================================================================
//  borrow_manager.h  ——  图书借阅管理者
// ===========================================================================
#pragma once

#include "model/book.h"
#include "model/student.h"
#include "util/date.h"

#include <string>
#include <vector>

namespace borrow {

// 统一返回结果：是否成功 + 提示信息
struct Result {
    bool ok{false};
    std::string message;
    static Result success(std::string text) { return Result{true, std::move(text)}; }
    static Result failure(std::string text) { return Result{false, std::move(text)}; }
};

// 归还结果
struct ReturnResult {
    bool ok{false};
    std::string message;
    long long overdue_days{0};
    double fine{0.0};
};

class BorrowManager {
public:
    BorrowManager() = default;

    // ---------------------------- 组合----------------------------
    // 图书入库 / 学生注册（管理者拥有这些对象）
    void add_book(const Book& book);
    void add_student(const Student& student);

    // 查询（返回指针表示"可空"，引用表示"一定存在"）
    Book* find_book(const std::string& isbn);
    const Book* find_book(const std::string& isbn) const;
    Student* find_student(const std::string& id);
    const Student* find_student(const std::string& id) const;

    const std::vector<Book>& books() const noexcept { return books_; }
    const std::vector<Student>& students() const noexcept { return students_; }

    // ---------------------------- 依赖：借书 ----------------------------
    Result borrow_book(Student& student, Book& book, const Date& date);

    // ---------------------------- 依赖：还书 ----------------------------
    // 还书：按 ISBN 归还；自动判断是否逾期并计算罚款
    ReturnResult return_book(Student& student, const std::string& isbn, const Date& date);

    // 预估罚款（不修改任何状态，供界面预览）
    long long overdue_days(const Student& student, const std::string& isbn, const Date& date) const;
    double preview_fine(const Student& student, const std::string& isbn, const Date& date) const;

    // ---------------------------- 系统日期 ----------------------------
    const Date& today() const noexcept { return today_; }
    void set_today(const Date& date) { today_ = date; }
    Date advance_days(long long days);

    // ---------------------------- 数据持久化 ----------------------------
    void load(const std::string& dir);   // 读取数据文件（不存在的文件按空处理）
    void save(const std::string& dir) const;

private:
    std::vector<Book> books_;        // 图书由管理者拥有
    std::vector<Student> students_;  // 学生由管理者拥有
    Date today_{Date::today()};
};

}  // namespace borrow
