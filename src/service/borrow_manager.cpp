// ===========================================================================
//  borrow_manager.cpp  ——  借书 / 还书 核心业务实现
//  ---------------------------------------------------------------------------
//  说明：
//   * 标有【请自行实现】的代码块是本题的核心算法，需要你补充自己的实现与注释；
//   * 标有【异常处理·占位符】的位置需要你补充自己的异常处理代码，
//     报告"五、测试与调试 / 运行时错误"部分记得插入带当前时间的调试截图。
// ===========================================================================
#include "borrow_manager.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

namespace borrow {

// ===========================================================================
//  组合部分：管理者持有学生与图书对象
// ===========================================================================
void BorrowManager::add_book(const Book& book) {
    if (find_book(book.isbn()) != nullptr) {
        throw std::invalid_argument(std::format("图书已存在：{}", book.isbn()));
    }
    books_.push_back(book);
}

void BorrowManager::add_student(const Student& student) {
    if (find_student(student.id()) != nullptr) {
        throw std::invalid_argument(std::format("学生已存在：{}", student.id()));
    }
    students_.push_back(student);
}

Book* BorrowManager::find_book(const std::string& isbn) {
    const auto it = std::ranges::find_if(books_, [&isbn](const Book& b) { return b.isbn() == isbn; });
    return it == books_.end() ? nullptr : &*it;
}

const Book* BorrowManager::find_book(const std::string& isbn) const {
    const auto it = std::ranges::find_if(books_, [&isbn](const Book& b) { return b.isbn() == isbn; });
    return it == books_.end() ? nullptr : &*it;
}

Student* BorrowManager::find_student(const std::string& id) {
    const auto it =
        std::ranges::find_if(students_, [&id](const Student& s) { return s.id() == id; });
    return it == students_.end() ? nullptr : &*it;
}

const Student* BorrowManager::find_student(const std::string& id) const {
    const auto it =
        std::ranges::find_if(students_, [&id](const Student& s) { return s.id() == id; });
    return it == students_.end() ? nullptr : &*it;
}

// ===========================================================================
//  ★ 核心函数：借书（依赖关系 + 双方状态判断 + 双方状态同步变更）
// ===========================================================================
Result BorrowManager::borrow_book(Student& student, Book& book, const Date& date) {
    // ===================== 【请自行实现】(7/7 · 主算法) =====================
    // 【核心算法·借书主流程】—— 报告"四、核心代码实现"重点展示这一段
    // TODO(你的实现)：按下面的流程补全代码，并写上你自己的注释：
    //
    //  第 1 步：学生状态判断
    //      - student.can_borrow_more() 为 false => 提示"已达最大借阅数量"
    //      - student.has_borrowed(book.isbn()) => 提示"不能重复借同一本书"
    //  第 2 步：图书状态判断
    //      - book.borrowable() 为 false => 提示"图书已全部借出"
    //  第 3 步：两者状态都允许，同步改变双方状态（注意参数是引用，改的是同一个对象）
    //      - Date due = date.add_days(Student::kBorrowDays);
    //      - book.take_one();                        // 图书：在馆册数 -1
    //      - student.add_borrowed_item(&book, date, due);  // 学生：登记借阅清单
    //  第 4 步：返回 Result::success(...)，消息里带上应还日期
    //
    //  进阶（可选，报告可写）：状态改变只做一次、失败不留半成品——
    //      若第三步中某一步抛异常，需要把已经改掉的状态回滚（try/catch + 回滚）。
    // ==============================================================
    (void)student;
    (void)book;
    (void)date;
    return Result::failure("【请自行实现】借书主流程尚未填写（见 borrow_manager.cpp 的 (7/7) 标记）");

    // -------------------------------------------------------------------
    // 【参考实现（已注释，供你对比思路；请自行写完后删除本块）】
    // 说明：这套逻辑已经跑通全部 7 个演示场景，可作为验收对照；
    //       直接照抄进报告不符合"需包含自己的修改注释"的要求，
    //       建议先自己写，再对照这里的差异。
    // -------------------------------------------------------------------
    // if (!student.can_borrow_more()) {
    //     return Result::failure(std::format("借书失败：{} 已借 {} 本，达到最大借阅数量 {} 本",
    //                                        student.name(), student.borrowed_count(),
    //                                        student.max_borrow()));
    // }
    // if (student.has_borrowed(book.isbn())) {
    //     return Result::failure(
    //         std::format("借书失败：{} 已借过《{}》，不能重复借阅", student.name(), book.title()));
    // }
    // if (!book.borrowable()) {
    //     return Result::failure(std::format("借书失败：《{}》已全部借出（在馆 {} 册）", book.title(),
    //                                        book.available()));
    // }
    // const Date due = date.add_days(Student::kBorrowDays);
    // book.take_one();
    // student.add_borrowed_item(&book, date, due);
    // return Result::success(std::format("借书成功：{} 借出《{}》，借书日 {}，应还日 {}（借期 {} 天）",
    //                                    student.name(), book.title(), date.to_string(),
    //                                    due.to_string(), Student::kBorrowDays));
}

// ===========================================================================
//  ★ 还书：登记归还 + 逾期判定 + 罚款计算
// ===========================================================================
ReturnResult BorrowManager::return_book(Student& student, const std::string& isbn, const Date& date) {
    // 1) 学生是否借过这本书
    const Student::BorrowItem* item = student.find_item(isbn);
    if (item == nullptr) {
        return ReturnResult{false, std::format("该学生没有借阅图书 {}，无法归还", isbn), 0, 0.0};
    }

    // 2) 逾期判定：应还日 vs 实际归还日
    const long long days = item->due_date.days_until(date);
    const long long overdue = days > 0 ? days : 0;
    const double fine = static_cast<double>(overdue) * Student::kFinePerDay;

    // 3) 同步改变双方状态：图书在馆册数 +1，学生借阅清单移除该书
    const bool removed = student.remove_borrowed_item(isbn);
    if (!removed) {
        return ReturnResult{false, std::format("归还失败：借阅清单中未找到 {}", isbn), 0, 0.0};
    }
    if (Book* book = find_book(isbn)) {
        book->give_one();
    }

    // 4) 罚款计入学生累计罚款，并组织返回信息
    if (overdue > 0) student.add_fine(fine);

    const std::string message =
        overdue > 0 ? std::format("归还成功：逾期 {} 天，罚款 {:.2f} 元", overdue, fine)
                    : std::string{"归还成功：按期归还，无罚款"};
    return ReturnResult{true, message, overdue, fine};
}

long long BorrowManager::overdue_days(const Student& student, const std::string& isbn,
                                      const Date& date) const {
    const Student::BorrowItem* item = student.find_item(isbn);
    if (item == nullptr) return 0;
    const long long days = item->due_date.days_until(date);
    return days > 0 ? days : 0;
}

double BorrowManager::preview_fine(const Student& student, const std::string& isbn,
                                   const Date& date) const {
    return static_cast<double>(overdue_days(student, isbn, date)) * Student::kFinePerDay;
}

Date BorrowManager::advance_days(long long days) {
    today_ = today_.add_days(days);
    return today_;
}

// ===========================================================================
//  数据持久化（文件存储）
// ===========================================================================
void BorrowManager::load(const std::string& dir) {
    books_.clear();
    students_.clear();

    // ---- 读取图书：ISBN|书名|作者|总册数|在馆册数|借出次数 ----
    std::ifstream book_file(dir + "/books.txt");
    if (book_file) {
        std::string line;
        while (std::getline(book_file, line)) {
            if (line.empty() || line[0] == '#') continue;
            // ================ 【异常处理·占位符】(A) ================
            // TODO(你的异常处理)：数据文件可能被手工改坏（字段个数不对、
            //      数字位置写了字母），请在这里补充你的解析与容错代码，例如：
            //        用 std::stoi 解析数字并 catch (const std::exception&)
            //        字段不足时跳过该行并打印"第 N 行数据损坏，已跳过"
            //      并在报告中插入此时的控制台/调试截图（含当前时间）。
            // ======================================================
            const std::size_t p1 = line.find('|');
            const std::size_t p2 = line.find('|', p1 + 1);
            const std::size_t p3 = line.find('|', p2 + 1);
            const std::size_t p4 = line.find('|', p3 + 1);
            const std::size_t p5 = line.find('|', p4 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos ||
                p4 == std::string::npos || p5 == std::string::npos) {
                continue;  // 容错：字段不足则跳过
            }
            Book book{line.substr(0, p1), line.substr(p1 + 1, p2 - p1 - 1),
                      line.substr(p2 + 1, p3 - p2 - 1)};
            book.set_total(std::stoi(line.substr(p3 + 1, p4 - p3 - 1)));
            book.set_available(std::stoi(line.substr(p4 + 1, p5 - p4 - 1)));
            book.set_borrow_count(std::stoi(line.substr(p5 + 1)));
            books_.push_back(book);
        }
    }

    // ---- 读取学生：学号|姓名|专业|最大借阅数量|累计罚款 ----
    std::ifstream student_file(dir + "/students.txt");
    if (student_file) {
        std::string line;
        while (std::getline(student_file, line)) {
            if (line.empty() || line[0] == '#') continue;
            const std::size_t p1 = line.find('|');
            const std::size_t p2 = line.find('|', p1 + 1);
            const std::size_t p3 = line.find('|', p2 + 1);
            const std::size_t p4 = line.find('|', p3 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos ||
                p4 == std::string::npos) {
                continue;
            }
            Student student{line.substr(0, p1), line.substr(p1 + 1, p2 - p1 - 1),
                            line.substr(p2 + 1, p3 - p2 - 1)};
            student.set_max_borrow(std::stoi(line.substr(p3 + 1, p4 - p3 - 1)));
            student.add_fine(std::stod(line.substr(p4 + 1)));
            students_.push_back(student);
        }
    }
}

void BorrowManager::save(const std::string& dir) const {
    // ================ 【异常处理·占位符】(B) ================
    // TODO(你的异常处理)：写文件可能失败（目录不存在 / 文件被占用 / 磁盘满）。
    //      请在这里补充你的处理代码，例如：
    //        写完后检查 ofs.good()，失败时抛 std::runtime_error("保存失败: ...")
    //      并在调用处 try/catch 打印友好提示；报告中插入截图。
    // ======================================================
    {
        std::ofstream ofs(dir + "/books.txt", std::ios::trunc);
        for (const Book& book : books_) {
            ofs << std::format("{}|{}|{}|{}|{}|{}", book.isbn(), book.title(), book.author(),
                               book.total(), book.available(), book.borrow_count())
                << '\n';
        }
    }
    {
        std::ofstream ofs(dir + "/students.txt", std::ios::trunc);
        for (const Student& student : students_) {
            ofs << std::format("{}|{}|{}|{}|{:.2f}", student.id(), student.name(), student.major(),
                               student.max_borrow(), student.total_fine())
                << '\n';
        }
    }
}

}  // namespace borrow
