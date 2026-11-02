// ===========================================================================
//  borrow_manager.h  ——  借阅管理者（业务核心）
//  ---------------------------------------------------------------------------
//  实验归属：
//   【实验2·组合与依赖（延续）】BorrowManager 组合持有仓储对象，
//                              借书/还书以 User& / Book& 引用传参体现依赖；
//   【实验3·继承与派生】通过 User* 统一管理 Student / Teacher 两种派生对象；
//   【实验4·多态性与虚函数】借阅上限、借期、罚款单价全部由虚函数给出，
//                              新增角色（如教师）无需修改借还算法；
//   【实验5·模板使用】三个成员都是 Repository<T, KeyOf> 模板的实例；
//   【实验6·输入输出流】load()/save() 使用 std::ifstream / std::ofstream
//                              与 operator>> / serialize() 完成持久化。
// ===========================================================================
#pragma once

#include "model/book.h"
#include "model/borrow_record.h"
#include "model/user.h"
#include "repository/repository.h"
#include "util/date.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace borrow {

// 【实验5·模板使用】用同一套 Repository 模板管理三种不同实体：
//   用户是多态对象 -> 存 std::unique_ptr<User>，主键策略用 PointerKey；
//   图书/记录是值对象 -> 直接存值，主键策略用默认的 DirectKey。
using UserRepository = Repository<std::unique_ptr<User>, PointerKey>;
using BookRepository = Repository<Book>;
using RecordRepository = Repository<BorrowRecord>;

// 统一操作结果：成功与否 + 提示信息
struct Result {
    bool ok{false};
    std::string message;

    static Result success(std::string text) { return Result{true, std::move(text)}; }
    static Result failure(std::string text) { return Result{false, std::move(text)}; }
};

// 还书结果：额外带回逾期天数与罚款
struct ReturnResult {
    bool ok{false};
    std::string message;
    long long overdue_days{0};
    double fine{0.0};
};

class BorrowManager {
public:
    explicit BorrowManager(std::filesystem::path data_dir);

    // ---------------------------- 数据文件 ----------------------------
    void load();
    void save() const;                       // 写回 users.txt / books.txt / records.txt
    [[nodiscard]] bool empty() const;        // 三个数据文件都为空
    [[nodiscard]] bool seed_sample_data();   // 首次运行写入演示数据
    [[nodiscard]] const std::filesystem::path& data_dir() const noexcept { return data_dir_; }

    // ---------------------------- 用户管理 ----------------------------
    [[nodiscard]] const UserRepository& users() const noexcept { return users_; }
    [[nodiscard]] std::vector<const User*> list_users() const;   // 按编号排序的基类指针
    [[nodiscard]] User* find_user(const std::string& id);
    [[nodiscard]] const User* find_user(const std::string& id) const;
    [[nodiscard]] std::vector<const User*> search_users(const std::string& keyword) const;

    Result add_student(std::string id, std::string name, std::string department,
                       std::string major, std::string grade,
                       int max_borrow = Student::kDefaultMaxBorrow);
    Result add_teacher(std::string id, std::string name, std::string department,
                       std::string title,
                       int max_borrow = Teacher::kDefaultMaxBorrow);
    Result remove_user(const std::string& id);   // 有未还图书时拒绝删除

    // ---------------------------- 图书管理 ----------------------------
    [[nodiscard]] const BookRepository& books() const noexcept { return books_; }
    [[nodiscard]] std::vector<const Book*> list_books() const;
    [[nodiscard]] Book* find_book(const std::string& isbn);
    [[nodiscard]] const Book* find_book(const std::string& isbn) const;
    [[nodiscard]] std::vector<const Book*> search_books(const std::string& keyword) const;

    Result add_book(std::string isbn, std::string title, std::string author, int total = 1);
    Result remove_book(const std::string& isbn);  // 借出中时拒绝删除

    // ---------------------------- 借书 / 还书 ----------------------------
    // 借书：用户与图书均以引用传入；返回的 message 中含系统计算出的应还日期
    Result borrow_book(User& user, Book& book, const Date& date);
    ReturnResult return_book(User& user, const std::string& isbn, const Date& date);

    [[nodiscard]] long long overdue_days(const User& user, const std::string& isbn,
                                         const Date& date) const;
    [[nodiscard]] double preview_fine(const User& user, const std::string& isbn,
                                      const Date& date) const;

    // ---------------------------- 借阅记录 / 统计 ----------------------------
    [[nodiscard]] const RecordRepository& records() const noexcept { return records_; }
    [[nodiscard]] std::vector<const BorrowRecord*> all_records() const;
    [[nodiscard]] std::vector<const BorrowRecord*> active_records() const;
    [[nodiscard]] std::vector<const BorrowRecord*> records_of_user(const std::string& user_id) const;
    [[nodiscard]] std::vector<const BorrowRecord*> active_records_of_user(
        const std::string& user_id) const;
    [[nodiscard]] std::vector<const BorrowRecord*> overdue_records(const Date& date) const;
    [[nodiscard]] const BorrowRecord* find_active_record(const std::string& user_id,
                                                         const std::string& isbn) const;
    [[nodiscard]] int active_count_of_user(const std::string& user_id) const;
    [[nodiscard]] double total_fine_of_user(const std::string& user_id) const;

    // ---------------------------- 系统日期 ----------------------------
    [[nodiscard]] const Date& today() const noexcept { return today_; }
    void set_today(const Date& date) { today_ = date; }
    Date advance_days(long long days);

private:
    [[nodiscard]] std::filesystem::path path_of(const std::string& file_name) const;
    [[nodiscard]] std::string next_record_id() const;  // R00001 形式
    void rebuild_user_borrows();                        // 加载后把在借记录挂回用户

    std::filesystem::path data_dir_;
    UserRepository users_;      // 【实验5】模板仓储：多态用户
    BookRepository books_;      // 【实验5】模板仓储：图书
    RecordRepository records_;  // 【实验5】模板仓储：借阅记录
    Date today_{Date::today()};
    int record_seq_{0};         // 借阅流水号自增序列
};

}  // namespace borrow
