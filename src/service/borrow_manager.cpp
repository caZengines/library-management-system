// ===========================================================================
//  borrow_manager.cpp  ——  借阅业务实现
// ===========================================================================
#include "borrow_manager.h"

#include "util/strings.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace borrow {

// ===========================================================================
//  构造与数据文件
// ===========================================================================
BorrowManager::BorrowManager(std::filesystem::path data_dir)
    : data_dir_(std::move(data_dir)), today_(Date::today()) {
    load();
}

std::filesystem::path BorrowManager::path_of(const std::string& file_name) const {
    return data_dir_ / file_name;
}

// 读文件：ifstream + operator>> / getline 
void BorrowManager::load() {
    users_.clear();
    books_.clear();
    records_.clear();

    // ---- 图书：每行一条，operator>> 负责解析并跳过损坏行 ----
    if (std::ifstream in{path_of("books.txt")}) {
        Book book;
        while (in >> book) {
            try {
                books_.add(book);
            } catch (const std::exception& e) {
                std::cerr << "[loading] 图书重复，已跳过：" << e.what() << '\n';
            }
        }
    }

    // ---- 用户：多态工厂按 STUDENT/TEACHER 构造对应派生类 ----
    if (std::ifstream in{path_of("users.txt")}) {
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::string cleaned = trim(line);
            if (cleaned.empty() || cleaned.front() == '#') continue;
            auto user = User::deserialize(cleaned);
            if (!user) {
                std::cerr << "[loading] 用户数据行损坏，已跳过：" << cleaned << '\n';
                continue;
            }
            try {
                users_.add(std::move(*user));
            } catch (const std::exception& e) {
                std::cerr << "[loading] " << e.what() << "，已跳过\n";
            }
        }
    }

    // ---- 借阅记录 ----
    if (std::ifstream in{path_of("records.txt")}) {
        BorrowRecord record;
        while (in >> record) {
            try {
                records_.add(record);
            } catch (const std::exception& e) {
                std::cerr << "[loading] 借阅记录重复，已跳过：" << e.what() << '\n';
            }
        }
    }

    // ---- 恢复借阅流水号序列：取已有记录号 R00007 中的数字最大值 ----
    record_seq_ = 0;
    for (const BorrowRecord& record : records_.all()) {
        if (record.id().size() > 1 && record.id().front() == 'R') {
            try {
                record_seq_ = std::max(record_seq_, std::stoi(record.id().substr(1)));
            } catch (const std::exception&) {
                // 记录号格式不规范，忽略
            }
        }
    }

    rebuild_user_borrows();
}

// 加载后：把"未归还"的记录重新挂到对应用户的借阅清单上
// （用户对象与图书对象分别来自 users.txt / books.txt，这里只恢复关联关系）
void BorrowManager::rebuild_user_borrows() {
    for (const BorrowRecord* record :
         records_.where([](const BorrowRecord& r) { return !r.returned(); })) {
        User* user = find_user(record->user_id());
        Book* book = find_book(record->book_isbn());
        if (user == nullptr || book == nullptr) continue;
        if (!user->has_borrowed(book->isbn())) {
            user->add_borrowed_item(book, record->borrow_date(), record->due_date());
        }
    }
}

// 写文件：ofstream + 格式化文本
void BorrowManager::save() const {
    std::error_code ec;
    std::filesystem::create_directories(data_dir_, ec);  // 目录不存在则创建

    {
        std::ofstream out{path_of("users.txt"), std::ios::trunc};
        if (!out) throw std::runtime_error("无法写入 users.txt");
        out << "# users.txt —— 每行一条用户记录，字段以 | 分隔\n";
        out << "# STUDENT|学号|姓名|院系|专业|年级|最大借阅数量|累计罚款\n";
        out << "# TEACHER|工号|姓名|院系|职称|最大借阅数量|累计罚款\n";
        for (const auto& user : users_.all()) out << user->serialize() << '\n';
        if (!out) throw std::runtime_error("写入 users.txt 失败");
    }
    {
        std::ofstream out{path_of("books.txt"), std::ios::trunc};
        if (!out) throw std::runtime_error("无法写入 books.txt");
        out << "# books.txt —— 每行一条图书记录，字段以 | 分隔\n";
        out << "# ISBN|书名|作者|总册数|在馆册数|借出次数\n";
        for (const Book& book : books_.all()) out << book.serialize() << '\n';
        if (!out) throw std::runtime_error("写入 books.txt 失败");
    }
    {
        std::ofstream out{path_of("records.txt"), std::ios::trunc};
        if (!out) throw std::runtime_error("无法写入 records.txt");
        out << "# records.txt —— 每行一条借阅记录，字段以 | 分隔\n";
        out << "# 记录号|借阅人|ISBN|借书日|应还日|归还日(-未还)|罚款|是否归还(1/0)\n";
        for (const BorrowRecord& record : records_.all()) out << record.serialize() << '\n';
        if (!out) throw std::runtime_error("写入 records.txt 失败");
    }
}

bool BorrowManager::empty() const {
    return users_.empty() && books_.empty() && records_.empty();
}

// ===========================================================================
//  演示数据
// ===========================================================================
bool BorrowManager::seed_sample_data() {
    if (!empty()) return false;

    add_student("2023001", "张三", "计算机学院", "软件工程", "2023级");
    add_student("2023002", "李四", "计算机学院", "人工智能", "2023级");
    add_student("2022015", "王五", "外国语学院", "英语", "2022级");
    add_teacher("T1001", "刘老师", "计算机学院", "副教授");
    add_teacher("T1002", "陈老师", "数学学院", "讲师");

    add_book("978-7-111-40701-4", "C++ Primer（第5版）", "Stanley B. Lippman", 3);
    add_book("978-7-121-35685-4", "深入理解计算机系统", "Randal E. Bryant", 1);
    add_book("978-7-302-53112-0", "数据结构（C++版）", "王红梅", 2);
    add_book("978-7-111-49997-9", "算法导论（第3版）", "Thomas H. Cormen", 2);

    // 一条正常在借、一条已逾期、一条已按期归还
    if (User* user = find_user("2023001")) {
        if (Book* book = find_book("978-7-111-40701-4")) {
            borrow_book(*user, *book, today_.add_days(-5));
        }
    }
    if (User* user = find_user("2023002")) {
        if (Book* book = find_book("978-7-121-35685-4")) {
            borrow_book(*user, *book, today_.add_days(-40));  // 学生借期 30 天 -> 逾期 10 天
        }
    }
    if (User* user = find_user("T1001")) {  // 教师借期 60 天
        if (Book* book = find_book("978-7-302-53112-0")) {
            borrow_book(*user, *book, today_.add_days(-70));
            return_book(*user, book->isbn(), today_.add_days(-35));  // 早于应还日 -> 无罚款
        }
    }
    save();
    return true;
}

// ===========================================================================
//  用户管理
// ===========================================================================
std::vector<const User*> BorrowManager::list_users() const {
    std::vector<const User*> result;
    result.reserve(users_.size());
    for (const auto& user : users_.all()) result.push_back(user.get());
    std::ranges::sort(result, {}, [](const User* user) { return user->id(); });
    return result;
}

User* BorrowManager::find_user(const std::string& id) {
    auto* found = users_.find(id);  // std::unique_ptr<User>*
    return found == nullptr ? nullptr : found->get();
}

const User* BorrowManager::find_user(const std::string& id) const {
    const auto* found = users_.find(id);
    return found == nullptr ? nullptr : found->get();
}

std::vector<const User*> BorrowManager::search_users(const std::string& keyword) const {
    std::vector<const User*> result;
    for (const auto& user : users_.all()) {
        const std::string text = user->id() + " " + user->name() + " " + user->department() + " " +
                                 user->extra_info();
        if (contains_ignore_case(text, keyword)) result.push_back(user.get());
    }
    std::ranges::sort(result, {}, [](const User* user) { return user->id(); });
    return result;
}

Result BorrowManager::add_student(std::string id, std::string name, std::string department,
                                  std::string major, std::string grade, int max_borrow) {
    id = trim(id);
    name = trim(name);
    department = trim(department);
    if (id.empty() || name.empty() || department.empty()) {
        return Result::failure("学号、姓名、院系为必填项");
    }
    if (max_borrow <= 0) return Result::failure("最大借阅数量必须为正数");
    try {
        users_.add(std::make_unique<Student>(id, name, department, trim(major), trim(grade),
                                             max_borrow));
    } catch (const std::exception& e) {
        return Result::failure(e.what());
    }
    save();
    return Result::success(std::format("已添加学生：{} {}（限借 {} 本）", id, name, max_borrow));
}

Result BorrowManager::add_teacher(std::string id, std::string name, std::string department,
                                  std::string title, int max_borrow) {
    id = trim(id);
    name = trim(name);
    department = trim(department);
    if (id.empty() || name.empty() || department.empty()) {
        return Result::failure("工号、姓名、院系为必填项");
    }
    if (max_borrow <= 0) return Result::failure("最大借阅数量必须为正数");
    try {
        users_.add(std::make_unique<Teacher>(id, name, department, trim(title), max_borrow));
    } catch (const std::exception& e) {
        return Result::failure(e.what());
    }
    save();
    return Result::success(std::format("已添加教师：{} {}（限借 {} 本）", id, name, max_borrow));
}

Result BorrowManager::remove_user(const std::string& id) {
    const User* user = find_user(id);
    if (user == nullptr) return Result::failure(std::format("用户不存在：{}", id));
    const std::string name = user->name();  // 先拷贝：删除后 user 指针会悬空

    const int borrowing = active_count_of_user(id);
    if (borrowing > 0) {
        return Result::failure(std::format("该用户还有 {} 本图书未归还，不能删除", borrowing));
    }
    if (!users_.remove(id)) return Result::failure("删除失败：用户不存在");
    save();
    return Result::success(std::format("已删除用户：{} {}", id, name));
}

// ===========================================================================
//  图书管理
// ===========================================================================
std::vector<const Book*> BorrowManager::list_books() const {
    std::vector<const Book*> result;
    result.reserve(books_.size());
    for (const Book& book : books_.all()) result.push_back(&book);
    std::ranges::sort(result, {}, [](const Book* book) { return book->isbn(); });
    return result;
}

Book* BorrowManager::find_book(const std::string& isbn) { return books_.find(isbn); }

const Book* BorrowManager::find_book(const std::string& isbn) const { return books_.find(isbn); }

std::vector<const Book*> BorrowManager::search_books(const std::string& keyword) const {
    std::vector<const Book*> result;
    for (const Book& book : books_.all()) {
        const std::string text = book.isbn() + " " + book.title() + " " + book.author();
        if (contains_ignore_case(text, keyword)) result.push_back(&book);
    }
    std::ranges::sort(result, {}, [](const Book* book) { return book->isbn(); });
    return result;
}

Result BorrowManager::add_book(std::string isbn, std::string title, std::string author, int total) {
    isbn = trim(isbn);
    title = trim(title);
    author = trim(author);
    if (isbn.empty() || title.empty() || author.empty()) {
        return Result::failure("ISBN、书名、作者为必填项");
    }
    if (total <= 0) return Result::failure("馆藏册数必须为正数");
    try {
        books_.add(Book{isbn, title, author, total, total});
    } catch (const std::exception& e) {
        return Result::failure(e.what());
    }
    save();
    return Result::success(std::format("已添加图书：《{}》（{} 册）", title, total));
}

Result BorrowManager::remove_book(const std::string& isbn) {
    const Book* book = find_book(isbn);
    if (book == nullptr) return Result::failure(std::format("图书不存在：{}", isbn));
    const std::string title = book->title();  // 先拷贝：删除后 book 指针会悬空

    const auto active = records_.where([&isbn](const BorrowRecord& record) {
        return record.book_isbn() == isbn && !record.returned();
    });
    if (!active.empty()) {
        return Result::failure("该图书尚有未归还的借阅记录，不能删除");
    }
    if (!books_.remove(isbn)) return Result::failure("删除失败：图书不存在");
    save();
    return Result::success(std::format("已删除图书：《{}》", title));
}

// ===========================================================================
//   核心业务：借书
//  ---------------------------------------------------------------------------
//  流程： 重复借阅校验 ->  学生状态校验 ->  图书状态校验
//        ->  双方状态同步变更 ->  写入借阅记录 ->  落盘
//  其中"限额、借期"来自 User 的虚函数，"是否可借"来自 Book，
//  BorrowManager 只持有基类引用/指针，因此对新增角色完全透明（多态）。
// ===========================================================================
Result BorrowManager::borrow_book(User& user, Book& book, const Date& date) {
    //  同一本书不允许重复借
    if (user.has_borrowed(book.isbn())) {
        return Result::failure(std::format("借书失败：{} 已借过《{}》，不能重复借阅", user.name(),
                                           book.title()));
    }

    //  用户状态：是否达到最大借阅数量（max_borrow() 为虚函数）
    if (!user.can_borrow_more()) {
        return Result::failure(std::format("借书失败：{}（{}）已借 {}/{} 本，达到最大借阅数量",
                                           user.name(), user.role_name(), user.borrowed_count(),
                                           user.max_borrow()));
    }

    //  图书状态：是否还有在馆册数
    if (!book.borrowable()) {
        return Result::failure(std::format("借书失败：《{}》已全部借出（在馆 {} 册）", book.title(),
                                           book.available()));
    }

    //  应还日 = 借书日 + 借期（借期由虚函数给出：学生 30 天 / 教师 60 天）
    const Date due_date = date.add_days(user.borrow_days());

    //  先改图书状态，再登记用户清单与借阅记录；中途失败则回滚
    if (!book.take_one()) {
        return Result::failure(std::format("借书失败：《{}》在馆册数不足", book.title()));
    }
    try {
        user.add_borrowed_item(&book, date, due_date);
        records_.add(BorrowRecord{next_record_id(), user.id(), book.isbn(), date, due_date});
        ++record_seq_;
    } catch (const std::exception& e) {
        book.give_one();
        user.remove_borrowed_item(book.isbn());
        return Result::failure(std::format("借书失败：{}", e.what()));
    }

    //  落盘
    try {
        save();
    } catch (const std::exception& e) {
        return Result::failure(std::format("借书成功，但数据保存失败：{}", e.what()));
    }

    return Result::success(std::format("借书成功：{}（{}）借出《{}》，借书日 {}，应还日 {}（借期 {} 天）",
                                       user.name(), user.role_name(), book.title(),
                                       date.to_string(), due_date.to_string(),
                                       user.borrow_days()));
}

// ===========================================================================
//   核心业务：还书（逾期判定 & 罚款结算）
// ===========================================================================
ReturnResult BorrowManager::return_book(User& user, const std::string& isbn, const Date& date) {
    const User::BorrowItem* item = user.find_item(isbn);
    if (item == nullptr) {
        return ReturnResult{false, std::format("还书失败：{} 没有借阅图书 {}", user.name(), isbn), 0,
                            0.0};
    }

    const BorrowRecord* active = find_active_record(user.id(), isbn);
    if (active == nullptr) {
        return ReturnResult{false, "还书失败：未找到对应的在借记录（数据可能不一致）", 0, 0.0};
    }

    // 逾期天数 = 归还日 - 应还日（大于 0 才算逾期）；日单价由虚函数给出
    const long long days = item->due_date.days_until(date);
    const long long overdue = days > 0 ? days : 0;
    const double fine = static_cast<double>(overdue) * user.fine_per_day();

    // 先更新记录（若失败则不改动内存中的其他状态）
    BorrowRecord updated = *active;
    updated.set_returned(true);
    updated.set_return_date(date);
    updated.set_fine(fine);
    try {
        records_.update(updated);
    } catch (const std::exception& e) {
        return ReturnResult{false, std::format("还书失败：{}", e.what()), 0, 0.0};
    }

    // 同步双方状态
    user.remove_borrowed_item(isbn);
    if (Book* book = find_book(isbn)) book->give_one();
    if (overdue > 0) user.add_fine(fine);

    try {
        save();
    } catch (const std::exception& e) {
        return ReturnResult{false, std::format("还书成功，但数据保存失败：{}", e.what()), overdue,
                            fine};
    }

    const std::string message =
        overdue > 0 ? std::format("归还成功：{} 归还《{}》，逾期 {} 天，罚款 {:.2f} 元", user.name(),
                                  find_book(isbn) ? find_book(isbn)->title() : isbn, overdue, fine)
                    : std::format("归还成功：{} 归还《{}》，按期归还，无罚款", user.name(),
                                  find_book(isbn) ? find_book(isbn)->title() : isbn);
    return ReturnResult{true, message, overdue, fine};
}

long long BorrowManager::overdue_days(const User& user, const std::string& isbn,
                                      const Date& date) const {
    const User::BorrowItem* item = user.find_item(isbn);
    if (item == nullptr) return 0;
    const long long days = item->due_date.days_until(date);
    return days > 0 ? days : 0;
}

double BorrowManager::preview_fine(const User& user, const std::string& isbn,
                                   const Date& date) const {
    return static_cast<double>(overdue_days(user, isbn, date)) * user.fine_per_day();
}

// ===========================================================================
//  借阅记录与统计
// ===========================================================================
std::vector<const BorrowRecord*> BorrowManager::all_records() const {
    std::vector<const BorrowRecord*> result;
    result.reserve(records_.size());
    for (const BorrowRecord& record : records_.all()) result.push_back(&record);
    std::ranges::sort(result, {}, [](const BorrowRecord* record) { return record->id(); });
    return result;
}

std::vector<const BorrowRecord*> BorrowManager::active_records() const {
    return records_.where([](const BorrowRecord& record) { return !record.returned(); });
}

std::vector<const BorrowRecord*> BorrowManager::records_of_user(const std::string& user_id) const {
    return records_.where(
        [&user_id](const BorrowRecord& record) { return record.user_id() == user_id; });
}

std::vector<const BorrowRecord*> BorrowManager::active_records_of_user(
    const std::string& user_id) const {
    return records_.where([&user_id](const BorrowRecord& record) {
        return record.user_id() == user_id && !record.returned();
    });
}

std::vector<const BorrowRecord*> BorrowManager::overdue_records(const Date& date) const {
    return records_.where([&date](const BorrowRecord& record) {
        return !record.returned() && date > record.due_date();
    });
}

const BorrowRecord* BorrowManager::find_active_record(const std::string& user_id,
                                                      const std::string& isbn) const {
    return records_.find_if([&user_id, &isbn](const BorrowRecord& record) {
        return !record.returned() && record.user_id() == user_id && record.book_isbn() == isbn;
    });
}

int BorrowManager::active_count_of_user(const std::string& user_id) const {
    return static_cast<int>(active_records_of_user(user_id).size());
}

double BorrowManager::total_fine_of_user(const std::string& user_id) const {
    const User* user = find_user(user_id);
    return user == nullptr ? 0.0 : user->total_fine();
}

Date BorrowManager::advance_days(long long days) {
    today_ = today_.add_days(days);
    return today_;
}

std::string BorrowManager::next_record_id() const {
    return std::format("R{:05d}", record_seq_ + 1);
}

}  // namespace borrow
