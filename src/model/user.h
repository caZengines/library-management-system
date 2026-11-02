// ===========================================================================
//  user.h  ——  用户体系
//  ---------------------------------------------------------------------------
//       - User 是抽象基类，抽取学号/工号、姓名、院系、借阅清单等公共属性；
//       - Student / Teacher 公有派生，各自扩展专业年级 / 职称；
//       - 基类析构函数需要声明为 virtual，保证通过基类指针 delete 时能正确析构派生部分；
//       - 派生类使用 override 明确重写虚函数
// ===========================================================================
#pragma once

#include "util/date.h"

#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace borrow {

class Book;

class User {
public:
    // 借阅条目：书 + 借书日 + 应还日
    struct BorrowItem {
        Book* book{nullptr};  
        Date borrow_date{};    
        Date due_date{};   
    };

    User(std::string id, std::string name, std::string department);
    virtual ~User() = default;

    User(const User&) = delete;                // 用户对象不可且通常不必拷贝（避免借阅清单指针被复制）
    User& operator=(const User&) = delete;
    User(User&&) noexcept = default;
    User& operator=(User&&) noexcept = default;

    // ---------------------------- 公共属性 ----------------------------
    const std::string& id() const noexcept { return id_; }          // 学号/工号（仓储主键）
    const std::string& name() const noexcept { return name_; }
    const std::string& department() const noexcept { return department_; }

    void set_name(std::string name) { name_ = std::move(name); }
    void set_department(std::string department) { department_ = std::move(department); }

    // ---------------------- 纯虚接口 ----------------------
    // 角色名（学生/教师）
    virtual std::string role_name() const = 0;
    // 专有属性（专业年级 / 职称）
    virtual std::string extra_info() const = 0;
    // 最大借阅数量、借期、逾期日单价：由派生类给出不同策略
    virtual int max_borrow() const = 0;
    virtual void set_max_borrow(int value) = 0;
    virtual int borrow_days() const = 0;
    virtual double fine_per_day() const = 0;
    // 序列化为一行文本（工厂反向还原）
    virtual std::string serialize() const = 0;
    // 输出到任意流（operator<< 调用，派生可扩展）
    virtual std::ostream& print(std::ostream& out) const;

    static std::optional<std::unique_ptr<User>> deserialize(const std::string& line);

    // ---------------------------- 借阅状态 ----------------------------
    const std::vector<BorrowItem>& borrowed() const noexcept { return borrowed_; }
    int borrowed_count() const noexcept { return static_cast<int>(borrowed_.size()); }
    bool can_borrow_more() const { return borrowed_count() < max_borrow(); }  // 虚函数参与判断
    bool has_borrowed(const std::string& isbn) const noexcept;
    void add_borrowed_item(Book* book, const Date& borrow_date, const Date& due_date);
    bool remove_borrowed_item(const std::string& isbn);
    const BorrowItem* find_item(const std::string& isbn) const;

    double total_fine() const noexcept { return total_fine_; }
    void add_fine(double amount) { total_fine_ += amount; }

    virtual std::string to_string() const;

protected:
    std::string id_;                      // 学号 / 工号
    std::string name_;                    // 姓名
    std::string department_;              // 院系
    std::vector<BorrowItem> borrowed_;    // 当前借阅清单
    double total_fine_{0.0};              // 累计罚款
};

// 重载流插入运算符：转发到虚函数 print()
std::ostream& operator<<(std::ostream& out, const User& user);

// ------------------------------- 学生 ---------------------------------
class Student final : public User {
public:
    static constexpr int kDefaultMaxBorrow = 5;      // 最多借 5 本
    static constexpr int kBorrowDays = 30;           // 借期 30 天
    static constexpr double kFinePerDay = 0.20;      // 逾期 0.20 元/天

    Student(std::string id, std::string name, std::string department,
            std::string major = {}, std::string grade = {},
            int max_borrow = kDefaultMaxBorrow);

    std::string role_name() const override { return "学生"; }
    std::string extra_info() const override;
    int max_borrow() const override { return max_borrow_; }
    void set_max_borrow(int value) override { max_borrow_ = value; }
    int borrow_days() const override { return kBorrowDays; }
    double fine_per_day() const override { return kFinePerDay; }
    std::string serialize() const override;

    const std::string& major() const noexcept { return major_; }
    const std::string& grade() const noexcept { return grade_; }
    void set_major(std::string major) { major_ = std::move(major); }
    void set_grade(std::string grade) { grade_ = std::move(grade); }

private:
    std::string major_;                    // 专业
    std::string grade_;                    // 年级
    int max_borrow_{kDefaultMaxBorrow};    // 最大借阅数量
};

// ------------------------------- 教师 ---------------------------------
class Teacher final : public User {
public:
    static constexpr int kDefaultMaxBorrow = 10;
    static constexpr int kBorrowDays = 60;           
    static constexpr double kFinePerDay = 0.50;      

    Teacher(std::string id, std::string name, std::string department,
            std::string title = {}, int max_borrow = kDefaultMaxBorrow);

    std::string role_name() const override { return "教师"; }
    std::string extra_info() const override;
    int max_borrow() const override { return max_borrow_; }
    void set_max_borrow(int value) override { max_borrow_ = value; }
    int borrow_days() const override { return kBorrowDays; }
    double fine_per_day() const override { return kFinePerDay; }
    std::string serialize() const override;

    const std::string& title() const noexcept { return title_; }
    void set_title(std::string title) { title_ = std::move(title); }

private:
    std::string title_;                      // 职称
    int max_borrow_{kDefaultMaxBorrow};      // 最大借阅数量
};

}  // namespace borrow
