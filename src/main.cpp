// ===========================================================================
//  main.cpp  ——  学生借书演示
// ===========================================================================
#include "service/borrow_manager.h"

#include <exception>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

namespace {

using borrow::Book;
using borrow::BorrowManager;
using borrow::Date;
using borrow::Result;
using borrow::ReturnResult;
using borrow::Student;

void print_separator() { std::cout << std::string(78, '-') << '\n'; }

void print_header(const std::string& title) {
    std::cout << '\n';
    print_separator();
    std::cout << "  " << title << '\n';
    print_separator();
}

// 打印一次借书结果
void show_result(const std::string& scene, const Result& result) {
    std::cout << std::format("  [{}] {}\n", result.ok ? "借书成功" : "借书失败", result.message);
    std::cout << std::format("      （场景：{}）\n", scene);
}

// 打印一次还书结果
void show_return(const std::string& scene, const ReturnResult& result) {
    std::cout << std::format("  [{}] {}\n", result.ok ? "还书成功" : "还书失败", result.message);
    std::cout << std::format("      （场景：{}）\n", scene);
}

// 演示数据初始化
void init_demo_data(BorrowManager& manager) {
    manager.add_book(Book{"978-7-111-40701-4", "C++ Primer（第5版）", "Stanley B. Lippman", 3, 3});
    manager.add_book(Book{"978-7-121-35685-4", "深入理解计算机系统", "Randal E. Bryant", 1, 1});
    manager.add_book(Book{"978-7-302-53112-0", "数据结构（C++版）", "王红梅", 2, 2});

    manager.add_student(Student{"2023001", "张三", "软件工程", 2});  
    manager.add_student(Student{"2023002", "李四", "计算机科学与技术", 3});
    manager.add_student(Student{"2023003", "王五", "网络安全", 3});
}

void print_student_state(const BorrowManager& manager, const std::string& id) {
    const Student* student = manager.find_student(id);
    if (student == nullptr) return;
    std::cout << std::format("  学生状态：{} {} 已借 {}/{} 本，累计罚款 {:.2f} 元\n", student->id(),
                             student->name(), student->borrowed_count(), student->max_borrow(),
                             student->total_fine());
    for (const auto& item : student->borrowed()) {
        if (item.book == nullptr) continue;
        std::cout << std::format("      · 《{}》 借书日 {}  应还日 {}\n", item.book->title(),
                                 item.borrow_date.to_string(), item.due_date.to_string());
    }
}

void print_book_state(const BorrowManager& manager, const std::string& isbn) {
    const Book* book = manager.find_book(isbn);
    if (book == nullptr) return;
    std::cout << std::format("  图书状态：《{}》 {}\n", book->title(), book->status_text());
}

}  // namespace

int main() {
    system("chcp 65001");

    try {
        BorrowManager manager;
        init_demo_data(manager);

        print_header("学生借书演示 —— 组合关系（管理者持有学生/图书） + 依赖关系（借书传引用）");
        std::cout << std::format("  系统日期：{}\n", manager.today().to_string());
        std::cout << std::format("  共 {} 种图书、{} 名学生\n", manager.books().size(),
                                 manager.students().size());

        // ---------------- 正常借书成功 ----------------
        print_header("场景1 正常借书：张三 借《C++ Primer》");
        {
            Student* student = manager.find_student("2023001");
            Book* book = manager.find_book("978-7-111-40701-4");
            show_result("双方状态都允许", manager.borrow_book(*student, *book, manager.today()));
            print_student_state(manager, "2023001");
            print_book_state(manager, "978-7-111-40701-4");
        }

        {
            Student* student = manager.find_student("2023001");
            manager.borrow_book(*student, *manager.find_book("978-7-121-35685-4"), manager.today());
            manager.borrow_book(*student, *manager.find_book("978-7-302-53112-0"), manager.today());
        }
        print_header("场景2 学生状态不允许：张三 借第 3 本（已借满 2 本，上限 2 本）");
        {
            Student* student = manager.find_student("2023001");
            Book* book = manager.find_book("978-7-121-35685-4");
            std::cout << std::format("  借阅前：{} 已借 {}/{} 本\n", student->name(),
                                     student->borrowed_count(), student->max_borrow());
            show_result("超出最大借阅数量", manager.borrow_book(*student, *book, manager.today()));
            print_student_state(manager, "2023001");
        }

        // ---------------- 重复借同一本书 ----------------
        print_header("场景3 学生状态不允许：张三 重复借已借过的《C++ Primer》");
        {
            Student* student = manager.find_student("2023002");
            // 先让李四借走一本，再把上限提高，专门演示"重复借同一本"这一条
            Book* book = manager.find_book("978-7-302-53112-0");
            manager.borrow_book(*student, *book, manager.today());
            show_result("同一本书不能重复借", manager.borrow_book(*student, *book, manager.today()));
        }

        // ---------------- 图书已全部借出 ----------------
        print_header("场景 图书状态不允许：李四 借《深入理解计算机系统》（仅 1 册，已被张三借走）");
        {
            Student* student = manager.find_student("2023002");
            Book* book = manager.find_book("978-7-121-35685-4");
            std::cout << std::format("  借阅前图书状态：《{}》 {}\n", book->title(),
                                     book->status_text());
            show_result("图书在馆册数为 0", manager.borrow_book(*student, *book, manager.today()));
            print_book_state(manager, "978-7-121-35685-4");
        }

        // ---------------- 按期归还 ----------------
        print_header("场景5 按期归还：李四 归还《数据结构（C++版）》（借出 10 天后）");
        {
            manager.advance_days(10);
            std::cout << std::format("  当前系统日期：{}\n", manager.today().to_string());
            Student* student = manager.find_student("2023002");
            const std::string isbn = "978-7-302-53112-0";
            std::cout << std::format("  归还前预估：逾期 {} 天，罚款 {:.2f} 元\n",
                                     manager.overdue_days(*student, isbn, manager.today()),
                                     manager.preview_fine(*student, isbn, manager.today()));
            show_return("未超过应还日期", manager.return_book(*student, isbn, manager.today()));
            print_student_state(manager, "2023002");
            print_book_state(manager, isbn);
        }

        // ---------------- 逾期归还 ----------------
        print_header("场景6 逾期归还：张三 归还《C++ Primer》（借出满 45 天，超过借期 30 天）");
        {
            manager.advance_days(35);  // 累计 45 天
            std::cout << std::format("  当前系统日期：{}\n", manager.today().to_string());
            Student* student = manager.find_student("2023001");
            const std::string isbn = "978-7-111-40701-4";
            const long long days = manager.overdue_days(*student, isbn, manager.today());
            const double fine = manager.preview_fine(*student, isbn, manager.today());
            std::cout << std::format("  归还前预估：逾期 {} 天 × 0.20 元/天 = {:.2f} 元\n", days,
                                     fine);
            show_return("超过应还日期", manager.return_book(*student, isbn, manager.today()));
            print_student_state(manager, "2023001");
            print_book_state(manager, isbn);
        }

        // ---------------- 归还没借过的书 ----------------
        print_header("场景7 异常分支：王五 归还自己没借过的《算法导论》");
        {
            Student* student = manager.find_student("2023003");
            show_return("借阅清单中不存在该书",
                        manager.return_book(*student, "978-7-111-49997-9", manager.today()));
        }

        print_header("演示结束");
        std::cout << "  全部图书状态：\n";
        for (const Book& book : manager.books()) {
            std::cout << std::format("      《{}》 {}\n", book.title(), book.status_text());
        }
        std::cout << "  全部学生状态：\n";
        for (const Student& student : manager.students()) {
            std::cout << std::format("      {} {} 已借 {}/{} 本，累计罚款 {:.2f} 元\n", student.id(),
                                     student.name(), student.borrowed_count(), student.max_borrow(),
                                     student.total_fine());
        }
        system("pause");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[严重错误] " << e.what() << '\n';
        system("pause");
        return 1;
    }
}
