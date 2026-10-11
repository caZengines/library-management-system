// ===========================================================================
//  borrow_manager.cpp  ——  借书 / 还书 业务实现
// ===========================================================================
#include "borrow_manager.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
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
    books_.emplace_back(book);
}

void BorrowManager::add_student(const Student& student) {
    if (find_student(student.id()) != nullptr) {
        throw std::invalid_argument(std::format("学生已存在：{}", student.id()));
    }
    students_.emplace_back(student);
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
    (void)student;
    (void)book;
    (void)date;
    // 检查学生是否可借
     if (!student.can_borrow_more()) {
         return Result::failure(std::format("借书失败！：您已借 {} 本，已达到最大借阅数量 {} 本",
                                             student.borrowed_count(),
                                            student.max_borrow()));
     }
     if (student.has_borrowed(book.isbn())) {
         return Result::failure(
             std::format("借书失败！：您已借过《{}》，不能重复借阅", book.title()));
     }

     // 检查图书是否可借
     if (!book.borrowable()) {
         return Result::failure(std::format("借书失败！：《{}》已全部借出（在馆 {} 册）", book.title(),
                                            book.available()));
     }

     // 到这一步说明满足借阅条件，开始借书
     const Date due = date.add_days(Student::kBorrowDays);
     book.take_one();
     student.add_borrowed_item(&book, date, due);
     return Result::success(std::format("借书成功！：{} 借出《{}》，借书日 {}，应还日 {}（借期 {} 天）",
                                        student.name(), book.title(), date.to_string(),
                                        due.to_string(), Student::kBorrowDays));
}

// ===========================================================================
//  还书：登记归还 + 逾期判定 + 罚款计算
// ===========================================================================
ReturnResult BorrowManager::return_book(Student& student, const std::string& isbn, const Date& date) {
    // 学生是否借过这本书
    const Student::BorrowItem* item = student.find_item(isbn);
    if (item == nullptr) {
        return ReturnResult{false, std::format("该学生没有借阅图书 {}，无法归还", isbn), 0, 0.0};
    }

    // 检查是否预期
    const long long days = item->due_date.days_until(date);
    const long long overdue = days > 0 ? days : 0;
    const double fine = static_cast<double>(overdue) * Student::kFinePerDay;

    // 同步状态
    const bool removed = student.remove_borrowed_item(isbn);
    if (!removed) {
        return ReturnResult{false, std::format("归还失败！：借阅清单中未找到 {}", isbn), 0, 0.0};
    }
    if (Book* book = find_book(isbn)) {
        book->give_one();
    }

    // 计算罚款
    if (overdue > 0) student.add_fine(fine);

    const std::string message =
        overdue > 0 ? std::format("归还成功！：逾期 {} 天，罚款 {:.2f} 元", overdue, fine)
                    : std::string{"归还成功！：按期归还"};
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
//  文件存储保证数据持久
// ===========================================================================
void BorrowManager::load(const std::string& dir) {
    books_.clear();
    students_.clear();

    // ---- 读取图书：ISBN|书名|作者|总册数|在馆册数|借出次数 ----
    std::ifstream book_file(dir + "/books.txt");
    if (book_file) {
        std::string line;
        std::size_t line_no = 0; 
        while (std::getline(book_file, line)) {
            if (line.empty() || line[0] == '#') continue;
            // windows换行键还多了个\r, 需要移除
            if (!line.empty() && line.back() == '\r') line.pop_back();

            const std::size_t p1 = line.find('|');
            const std::size_t p2 = line.find('|', p1 + 1);
            const std::size_t p3 = line.find('|', p2 + 1);
            const std::size_t p4 = line.find('|', p3 + 1);
            const std::size_t p5 = line.find('|', p4 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos ||
                p4 == std::string::npos || p5 == std::string::npos) {
                std::cerr << "第 " << line_no << " 行数据损坏，已跳过\n";
                continue; 
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
    {    
        std::ofstream ofs(dir + "/books.txt", std::ios::trunc);
        if(!ofs.is_open()) {
            throw std::runtime_error("无法打开books.txt文件！");
        }
        for (const Book& book : books_) {
            ofs << std::format("{}|{}|{}|{}|{}|{}", book.isbn(), book.title(), book.author(),
                            book.total(), book.available(), book.borrow_count())
                << '\n';
        }
        ofs.close();
    }
    
    {
        std::ofstream ofs(dir + "/students.txt", std::ios::trunc);
        if(!ofs.is_open()) {
            throw std::runtime_error("无法打开students.txt文件！");
        }
        for (const Student& student : students_) {
            ofs << std::format("{}|{}|{}|{}|{:.2f}", student.id(), student.name(), student.major(),
                               student.max_borrow(), student.total_fine())
                << '\n';
        }
        ofs.close();
    }
}

}  // namespace borrow
