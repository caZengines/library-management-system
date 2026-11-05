// ===========================================================================
//  menu.cpp  ——  控制台菜单实现
// ===========================================================================
#include "menu.h"

#include "service/borrow_manager.h"
#include "util/io.h"
#include "util/table.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

namespace borrow {
namespace {

constexpr std::string_view kSeparator =
    "--------------------------------------------------------------------------------";

void print_result(const Result& result) {
    std::cout << (result.ok ? "  [成功] " : "  [失败] ") << result.message << '\n';
}

int read_int_or(const std::string& prompt, int fallback, int min, int max) {
    const std::string text = io::read_line(prompt);
    if (text.empty()) return fallback;
    int value = 0;
    const auto* first = text.data();
    const auto* last = first + text.size();
    if (auto [ptr, ec] = std::from_chars(first, last, value);
        ec == std::errc{} && ptr == last && value >= min && value <= max) {
        return value;
    }
    std::cout << "  [tips] 输入无效，使用默认值 " << fallback << '\n';
    return fallback;
}

std::string user_table_header() {
    return std::format("{:<10} {:<4} {:<10} {:<12} {:<16} {:>4} {:>4} {:>8}", "编号", "角色", "姓名",
                       "院系", "专有属性", "限借", "借期", "日罚款");
}

std::string book_table_header() {
    return std::format("{:<16} {:<28} {:<14} {:>5} {:>6} {:>6}", "ISBN", "书名", "作者", "总册",
                       "在馆", "借次");
}

std::string record_table_header() {
    return std::format("{:<8} {:<10} {:<16} {:<12} {:<12} {:<12} {:<10} {:>8}", "记录号",
                       "借阅人", "图书ISBN", "借书日", "应还日", "归还日", "状态", "罚款");
}

std::string record_row(const BorrowRecord& record, const Date& today) {
    return std::format("{:<8} {:<10} {:<16} {:<12} {:<12} {:<12} {:<10} {:>8.2f}",
                       record.id(), record.user_id(), record.book_isbn(),
                       record.borrow_date().to_string(), record.due_date().to_string(),
                       record.returned() ? record.return_date().to_string() : std::string{"-"},
                       record.status_text(today), record.fine());
}

}  // namespace

// ===========================================================================
//  主循环
// ===========================================================================
void Menu::run() {
    for (;;) {
        print_main_menu();
        switch (io::read_int("请选择 [0-5]: ", 0, 5)) {
            case 1: handle_users(); break;
            case 2: handle_books(); break;
            case 3: handle_borrow(); break;
            case 4: handle_reports(); break;
            case 5: handle_system(); break;
            case 0:
            default:
                std::cout << "\n数据已保存，感谢使用！\n";
                return;
        }
    }
}

void Menu::print_main_menu() const {
    const auto users = manager_.list_users();
    const auto books = manager_.list_books();
    const auto active = manager_.active_records();

    std::cout << "\n==============================================================================\n";
    std::cout << "                        图书馆借阅管理系统\n";
    std::cout << "==============================================================================\n";
    std::cout << std::format("  系统日期：{}   用户 {} 人   图书 {} 种   在借 {} 条\n",
                             manager_.today().to_string(), users.size(), books.size(),
                             active.size());
    std::cout << kSeparator << '\n';
    std::cout << "  1. 用户管理（学生 / 教师）\n";
    std::cout << "  2. 图书管理\n";
    std::cout << "  3. 借书 / 还书\n";
    std::cout << "  4. 查询与统计\n";
    std::cout << "  5. 系统设置（日期 / 数据文件）\n";
    std::cout << "  0. 保存并退出\n";
    std::cout << kSeparator << '\n';
}

// ===========================================================================
//  用户管理
// ===========================================================================
void Menu::handle_users() {
    for (;;) {
        std::cout << "\n" << kSeparator << '\n'
                  << "                        用户管理\n"
                  << kSeparator << '\n'
                  << "  1. 查看全部用户      2. 添加用户     3. 修改用户\n"
                  << "  4. 删除用户          5. 查询用户     0. 返回主菜单\n"
                  << kSeparator << '\n';
        switch (io::read_int("请选择 [0-5]: ", 0, 5)) {
            case 1: show_users(); break;
            case 2: add_user(); break;
            case 3: edit_user(); break;
            case 4: delete_user(); break;
            case 5: search_users(); break;
            case 0: default: return;
        }
    }
}

void Menu::show_users() {
    print_table(std::cout, "用户列表", manager_.list_users(), user_table_header(),
                [](const User& user) { return user.to_string(); });
}

void Menu::add_user() {
    std::cout << "\n请选择用户类型：1. 学生   2. 教师\n";
    const int type = io::read_int("请选择 [1-2]: ", 1, 2);

    const auto id = io::read_required("编号（学号 / 工号，空行取消）: ");
    if (!id) {
        std::cout << "已取消添加。\n";
        return;
    }
    const auto name = io::read_required("姓名: ");
    if (!name) {
        std::cout << "已取消添加。\n";
        return;
    }
    const auto department = io::read_required("院系: ");
    if (!department) {
        std::cout << "已取消添加。\n";
        return;
    }

    if (type == 1) {
        const std::string major = io::read_line("专业（可空）: ");
        const std::string grade = io::read_line("年级（可空）: ");
        const int max_borrow =
            read_int_or(std::format("最大借阅数量 [默认 {}]: ", Student::kDefaultMaxBorrow),
                        Student::kDefaultMaxBorrow, 1, 50);
        print_result(manager_.add_student(*id, *name, *department, major, grade, max_borrow));
    } else {
        const std::string title = io::read_line("职称（可空）: ");
        const int max_borrow =
            read_int_or(std::format("最大借阅数量 [默认 {}]: ", Teacher::kDefaultMaxBorrow),
                        Teacher::kDefaultMaxBorrow, 1, 50);
        print_result(manager_.add_teacher(*id, *name, *department, title, max_borrow));
    }
}

void Menu::edit_user() {
    const auto id = io::read_required("\n请输入要修改的用户编号: ");
    if (!id) return;
    User* user = manager_.find_user(*id);
    if (user == nullptr) {
        std::cout << "用户不存在：" << *id << '\n';
        return;
    }

    std::cout << "  当前信息：" << *user << '\n';
    std::cout << "  直接回车表示保持原值：\n";

    const std::string name = io::read_line(std::format("姓名 [{}]: ", user->name()));
    if (!name.empty()) user->set_name(name);
    const std::string department =
        io::read_line(std::format("院系 [{}]: ", user->department()));
    if (!department.empty()) user->set_department(department);

    // 【实验4·多态性与虚函数】用 dynamic_cast 判断对象的真实派生类型，
    // 只有学生才有专业/年级，只有教师才有职称。
    if (auto* student = dynamic_cast<Student*>(user)) {
        const std::string major = io::read_line(std::format("专业 [{}]: ", student->major()));
        if (!major.empty()) student->set_major(major);
        const std::string grade = io::read_line(std::format("年级 [{}]: ", student->grade()));
        if (!grade.empty()) student->set_grade(grade);
    } else if (auto* teacher = dynamic_cast<Teacher*>(user)) {
        const std::string title = io::read_line(std::format("职称 [{}]: ", teacher->title()));
        if (!title.empty()) teacher->set_title(title);
    }

    const int max_borrow = read_int_or(std::format("最大借阅数量 [{}]: ", user->max_borrow()),
                                       user->max_borrow(), 1, 50);
    user->set_max_borrow(max_borrow);  // 虚函数：派生类各自保存自己的上限

    try {
        manager_.save();
        std::cout << "  [成功] 用户信息已修改并保存\n";
    } catch (const std::exception& e) {
        std::cout << "  [失败] 保存失败：" << e.what() << '\n';
    }
}

void Menu::delete_user() {
    const auto id = io::read_required("\n请输入要删除的用户编号: ");
    if (!id) return;
    const User* user = manager_.find_user(*id);
    if (user == nullptr) {
        std::cout << "用户不存在：" << *id << '\n';
        return;
    }
    std::cout << "  " << *user << '\n';
    if (!io::read_yes_no(std::format("确认删除用户 {} ?", *id))) {
        std::cout << "已取消删除。\n";
        return;
    }
    print_result(manager_.remove_user(*id));
}

void Menu::search_users() {
    const auto keyword = io::read_required("\n请输入关键字（编号 / 姓名 / 院系 / 专业 / 职称）: ");
    if (!keyword) return;
    std::vector<const User*> found = manager_.search_users(*keyword);
    if (found.empty()) {
        std::cout << "没有匹配的用户。\n";
        return;
    }
    print_table(std::cout, "查询结果", found, user_table_header(),
                [](const User& user) { return user.to_string(); });
}

// ===========================================================================
//  图书管理
// ===========================================================================
void Menu::handle_books() {
    for (;;) {
        std::cout << "\n" << kSeparator << '\n'
                  << "                        图书管理\n"
                  << kSeparator << '\n'
                  << "  1. 查看全部图书      2. 添加图书     3. 修改图书\n"
                  << "  4. 删除图书          5. 查询图书     0. 返回主菜单\n"
                  << kSeparator << '\n';
        switch (io::read_int("请选择 [0-5]: ", 0, 5)) {
            case 1: show_books(); break;
            case 2: add_book(); break;
            case 3: edit_book(); break;
            case 4: delete_book(); break;
            case 5: search_books(); break;
            case 0: default: return;
        }
    }
}

void Menu::show_books() {
    print_table(std::cout, "图书列表", manager_.list_books(), book_table_header(),
                [](const Book& book) { return book.to_string(); });
}

void Menu::add_book() {
    std::cout << "\n添加图书（输入空行取消）\n";
    const auto isbn = io::read_required("ISBN: ");
    if (!isbn) return;
    const auto title = io::read_required("书名: ");
    if (!title) return;
    const auto author = io::read_required("作者: ");
    if (!author) return;
    const int total = read_int_or("馆藏总册数 [默认 1]: ", 1, 1, 1000);
    print_result(manager_.add_book(*isbn, *title, *author, total));
}

void Menu::edit_book() {
    const auto isbn = io::read_required("\n请输入要修改的图书 ISBN: ");
    if (!isbn) return;
    Book* book = manager_.find_book(*isbn);
    if (book == nullptr) {
        std::cout << "图书不存在：" << *isbn << '\n';
        return;
    }
    std::cout << "  当前信息：" << *book << '\n';

    const std::string title = io::read_line(std::format("书名 [{}]: ", book->title()));
    if (!title.empty()) book->set_title(title);
    const std::string author = io::read_line(std::format("作者 [{}]: ", book->author()));
    if (!author.empty()) book->set_author(author);

    const int borrowed = book->total() - book->available();
    const int new_total =
        read_int_or(std::format("馆藏总册数 [{}]: ", book->total()), book->total(), 1, 1000);
    if (new_total < borrowed) {
        std::cout << std::format("  [失败] 当前有 {} 册在借，总册数不能小于 {}\n", borrowed,
                                 borrowed);
    } else {
        book->set_total(new_total);
    }

    try {
        manager_.save();
        std::cout << "  [成功] 图书信息已修改并保存\n";
    } catch (const std::exception& e) {
        std::cout << "  [失败] 保存失败：" << e.what() << '\n';
    }
}

void Menu::delete_book() {
    const auto isbn = io::read_required("\n请输入要删除的图书 ISBN: ");
    if (!isbn) return;
    const Book* book = manager_.find_book(*isbn);
    if (book == nullptr) {
        std::cout << "图书不存在：" << *isbn << '\n';
        return;
    }
    std::cout << "  " << *book << '\n';
    if (!io::read_yes_no(std::format("确认删除图书 {} ?", *isbn))) {
        std::cout << "已取消删除。\n";
        return;
    }
    print_result(manager_.remove_book(*isbn));
}

void Menu::search_books() {
    const auto keyword = io::read_required("\n请输入关键字（ISBN / 书名 / 作者）: ");
    if (!keyword) return;
    std::vector<const Book*> found = manager_.search_books(*keyword);
    if (found.empty()) {
        std::cout << "没有匹配的图书。\n";
        return;
    }
    print_table(std::cout, "查询结果", found, book_table_header(),
                [](const Book& book) { return book.to_string(); });
}

// ===========================================================================
//  借书 / 还书
// ===========================================================================
void Menu::handle_borrow() {
    for (;;) {
        std::cout << "\n" << kSeparator << '\n'
                  << "                        借书 / 还书\n"
                  << kSeparator << '\n'
                  << std::format("  当前系统日期：{}\n", manager_.today().to_string())
                  << "  1. 借书              2. 还书          3. 在借清单\n"
                  << "  0. 返回主菜单\n"
                  << kSeparator << '\n';
        switch (io::read_int("请选择 [0-3]: ", 0, 3)) {
            case 1: borrow_book(); break;
            case 2: return_book(); break;
            case 3: show_active_records(); break;
            case 0: default: return;
        }
    }
}

void Menu::borrow_book() {
    std::cout << "\n借书登记\n";
    const auto id = io::read_required("借阅人编号（学号 / 工号）: ");
    if (!id) return;
    User* user = manager_.find_user(*id);
    if (user == nullptr) {
        std::cout << "借阅人不存在：" << *id << '\n';
        return;
    }
    std::cout << "  " << *user << '\n';

    const auto isbn = io::read_required("图书 ISBN: ");
    if (!isbn) return;
    Book* book = manager_.find_book(*isbn);
    if (book == nullptr) {
        std::cout << "图书不存在：" << *isbn << '\n';
        return;
    }
    std::cout << "  " << *book << '\n';

    print_result(manager_.borrow_book(*user, *book, manager_.today()));
}

void Menu::return_book() {
    std::cout << "\n还书登记\n";
    const auto id = io::read_required("借阅人编号: ");
    if (!id) return;
    User* user = manager_.find_user(*id);
    if (user == nullptr) {
        std::cout << "借阅人不存在：" << *id << '\n';
        return;
    }
    const auto isbn = io::read_required("图书 ISBN: ");
    if (!isbn) return;

    if (user->find_item(*isbn) == nullptr) {
        std::cout << "该用户没有借阅这本书，无法归还。\n";
        return;
    }

    const long long days = manager_.overdue_days(*user, *isbn, manager_.today());
    const double fine = manager_.preview_fine(*user, *isbn, manager_.today());
    std::cout << std::format("  归还前预览：逾期 {} 天，罚款 {:.2f} 元（{:.2f} 元/天）\n", days,
                             fine, user->fine_per_day());
    if (!io::read_yes_no("确认归还？")) {
        std::cout << "已取消还书。\n";
        return;
    }

    const ReturnResult result = manager_.return_book(*user, *isbn, manager_.today());
    std::cout << (result.ok ? "  [成功] " : "  [失败] ") << result.message << '\n';
}

void Menu::show_active_records() {
    const Date today = manager_.today();
    print_table(std::cout, "在借清单", manager_.active_records(), record_table_header(),
                [today](const BorrowRecord& record) { return record_row(record, today); });
}

// ===========================================================================
//  查询与统计
// ===========================================================================
void Menu::handle_reports() {
    for (;;) {
        std::cout << "\n" << kSeparator << '\n'
                  << "                        查询与统计\n"
                  << kSeparator << '\n'
                  << "  1. 全部借阅记录      2. 逾期清单       3. 用户借阅详情\n"
                  << "  4. 系统总览          0. 返回主菜单\n"
                  << kSeparator << '\n';
        switch (io::read_int("请选择 [0-4]: ", 0, 4)) {
            case 1: show_all_records(); break;
            case 2: show_overdue_report(); break;
            case 3: show_user_detail(); break;
            case 4: show_summary(); break;
            case 0: default: return;
        }
    }
}

void Menu::show_all_records() {
    const Date today = manager_.today();
    print_table(std::cout, "全部借阅记录", manager_.all_records(), record_table_header(),
                [today](const BorrowRecord& record) { return record_row(record, today); });
}

void Menu::show_overdue_report() {
    const Date today = manager_.today();
    const auto records = manager_.overdue_records(today);
    std::cout << std::format("\n逾期清单（系统日期 {}，共 {} 条）\n", today.to_string(),
                             records.size());
    if (records.empty()) {
        std::cout << "  （当前没有逾期记录）\n";
        return;
    }
    std::cout << kSeparator << '\n';
    std::cout << std::format("  {:<8} {:<8} {:<16} {:<12} {:>8} {:>8} {:>10}\n", "记录号",
                             "姓名", "图书ISBN", "应还日", "逾期天数", "日罚款", "应缴罚款");
    std::cout << kSeparator << '\n';
    double total = 0.0;
    for (const BorrowRecord* record : records) {
        const User* user = manager_.find_user(record->user_id());
        const long long days = record->overdue_days_now(today);
        const double rate = user != nullptr ? user->fine_per_day() : 0.0;
        const double fine = static_cast<double>(days) * rate;
        total += fine;
        std::cout << std::format("  {:<8} {:<8} {:<16} {:<12} {:>6}天 {:>8.2f} {:>10.2f}\n",
                                 record->id(), user != nullptr ? user->name() : record->user_id(),
                                 record->book_isbn(), record->due_date().to_string(), days, rate,
                                 fine);
    }
    std::cout << kSeparator << '\n';
    std::cout << std::format("  当前逾期罚款合计：{:.2f} 元\n", total);
}

void Menu::show_user_detail() {
    const auto id = io::read_required("\n请输入用户编号: ");
    if (!id) return;
    const User* user = manager_.find_user(*id);
    if (user == nullptr) {
        std::cout << "用户不存在：" << *id << '\n';
        return;
    }
    std::cout << "\n  " << *user << '\n';

    const auto history = manager_.records_of_user(*id);
    const auto active = manager_.active_records_of_user(*id);
    std::cout << std::format("\n  已借 {}/{} 本，累计借阅 {} 次，累计罚款 {:.2f} 元\n",
                             active.size(), user->max_borrow(), history.size(),
                             user->total_fine());

    const Date today = manager_.today();
    if (!active.empty()) {
        print_table(std::cout, "未归还图书", active, record_table_header(),
                    [today](const BorrowRecord& record) { return record_row(record, today); });
    }
    if (!history.empty()) {
        print_table(std::cout, "历史借阅记录", history, record_table_header(),
                    [today](const BorrowRecord& record) { return record_row(record, today); });
    }
}

void Menu::show_summary() {
    const auto users = manager_.list_users();
    const auto books = manager_.list_books();
    const auto all = manager_.all_records();
    const auto active = manager_.active_records();
    const Date today = manager_.today();

    const auto students =
        std::ranges::count_if(users, [](const User* user) {
            return dynamic_cast<const Student*>(user) != nullptr;
        });
    const auto teachers = static_cast<long long>(users.size()) - students;

    int total_copies = 0;
    int available_copies = 0;
    for (const Book* book : books) {
        total_copies += book->total();
        available_copies += book->available();
    }

    const auto overdue = manager_.overdue_records(today);
    double overdue_fine = 0.0;
    for (const BorrowRecord* record : overdue) {
        const User* user = manager_.find_user(record->user_id());
        overdue_fine += static_cast<double>(record->overdue_days_now(today)) *
                        (user != nullptr ? user->fine_per_day() : 0.0);
    }

    double collected_fine = 0.0;
    for (const BorrowRecord* record : all) {
        if (record->returned()) collected_fine += record->fine();
    }

    std::cout << "\n--------------------------- 系统总览 ---------------------------\n";
    std::cout << std::format("  系统日期：{}\n", today.to_string());
    std::cout << std::format("  用户：{} 人（学生 {}，教师 {}）\n", users.size(), students, teachers);
    std::cout << std::format("  图书：{} 种，共 {} 册（在馆 {} 册，借出 {} 册）\n", books.size(),
                             total_copies, available_copies, total_copies - available_copies);
    std::cout << std::format("  借阅记录：{} 条（在借 {}，已归还 {}）\n", all.size(), active.size(),
                             all.size() - active.size());
    std::cout << std::format("  逾期未还：{} 条，应收罚款 {:.2f} 元\n", overdue.size(),
                             overdue_fine);
    std::cout << std::format("  已收罚款（历史归还）：{:.2f} 元\n", collected_fine);
    std::cout << "----------------------------------------------------------------\n";
}

// ===========================================================================
//  系统设置
// ===========================================================================
void Menu::handle_system() {
    for (;;) {
        std::cout << "\n" << kSeparator << '\n'
                  << "                        系统设置\n"
                  << kSeparator << '\n'
                  << "  1. 查看数据文件位置    2. 修改系统日期    3. 系统日期快进\n"
                  << "  4. 立即保存数据        5. 重新载入数据    6. 重置为演示数据\n"
                  << "  0. 返回主菜单\n"
                  << kSeparator << '\n';
        switch (io::read_int("请选择 [0-6]: ", 0, 6)) {
            case 1:
                std::cout << std::format("\n  数据目录：{}\n", manager_.data_dir().string());
                std::cout << std::format("  用户文件：{}\n",
                                         (manager_.data_dir() / "users.txt").string());
                std::cout << std::format("  图书文件：{}\n",
                                         (manager_.data_dir() / "books.txt").string());
                std::cout << std::format("  记录文件：{}\n",
                                         (manager_.data_dir() / "records.txt").string());
                break;
            case 2: {
                const Date date = io::read_date("\n请输入系统日期 (YYYY-MM-DD): ");
                manager_.set_today(date);
                std::cout << std::format("  系统日期已设置为：{}\n", date.to_string());
                break;
            }
            case 3: {
                const int days = io::read_int("\n快进天数（负数表示回退）[-3650-3650]: ", -3650, 3650);
                std::cout << std::format("  系统日期已变为：{}\n",
                                         manager_.advance_days(days).to_string());
                break;
            }
            case 4:
                try {
                    manager_.save();
                    std::cout << "  [成功] 全部数据已保存\n";
                } catch (const std::exception& e) {
                    std::cout << "  [失败] " << e.what() << '\n';
                }
                break;
            case 5:
                manager_.load();
                std::cout << "  已从数据文件重新载入\n";
                break;
            case 6: {
                if (!io::read_yes_no("\n重置将清空当前数据并重新写入演示数据，确认？")) {
                    std::cout << "已取消。\n";
                    break;
                }
                std::error_code ec;
                for (const char* name : {"users.txt", "books.txt", "records.txt"}) {
                    std::filesystem::remove(manager_.data_dir() / name, ec);
                }
                manager_.load();
                if (manager_.seed_sample_data()) {
                    std::cout << "  [成功] 已重置为演示数据\n";
                } else {
                    std::cout << "  [失败] 重置失败\n";
                }
                break;
            }
            case 0:
            default:
                return;
        }
    }
}

}  // namespace borrow
