// ===========================================================================
//  borrow_manager.h  ——  图书借阅管理者（组合关系 + 依赖关系的汇合点）
//  ---------------------------------------------------------------------------
//  ★ 类间关系说明（答辩重点）★
//   组合关系（Composition）：BorrowManager "拥有" 学生表与图书表，
//        成员为 std::vector<Student> / std::vector<Book>（按值持有），
//        管理者析构时学生/图书对象随之销毁，生命周期完全由管理者控制，
//        因此是强组合（实心菱形 ◄◆）。
//   依赖关系（Dependency）：借书动作 borrow_book(Student&, Book&)
//        以【引用】传参，并且把 Book* 存入学生的借阅清单，
//        参数与局部数据都指向外部对象——管理者不拥有 Book，
//        只在使用期间依赖它（虚线箭头 ┄>）。
// ===========================================================================
#pragma once

#include "model/book.h"
#include "model/student.h"
#include "util/date.h"

#include <string>
#include <vector>

namespace borrow {

// 统一的业务返回结果：是否成功 + 提示信息（避免到处打印，便于测试）
struct Result {
    bool ok{false};
    std::string message;
    static Result success(std::string text) { return Result{true, std::move(text)}; }
    static Result failure(std::string text) { return Result{false, std::move(text)}; }
};

// 归还结果：额外带回逾期天数与罚款
struct ReturnResult {
    bool ok{false};
    std::string message;
    long long overdue_days{0};
    double fine{0.0};
};

class BorrowManager {
public:
    BorrowManager() = default;

    // ---------------------------- 组合：持有对象 ----------------------------
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
    // 借书：学生与图书均以【引用】传入（体现依赖关系）
    // 校验：①学生状态（能否再借、是否重复借同一本）
    //       ②图书状态（是否可借）
    //       两者都满足才借书成功，并同步改变两边状态
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
    std::vector<Book> books_;        // 组合：图书由管理者拥有
    std::vector<Student> students_;  // 组合：学生由管理者拥有
    Date today_{Date::today()};
};

}  // namespace borrow
