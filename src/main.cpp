// ===========================================================================
//  main.cpp  ——  程序入口
//  ---------------------------------------------------------------------------
//                        启动菜单（交互流）或 --demo 演示模式。
//  调用关系：main -> Menu(界面) -> BorrowManager(业务) -> Repository(泛型仓储) -> 数据文件
// ===========================================================================
#include "service/borrow_manager.h"
#include "ui/menu.h"

#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

using borrow::Book;
using borrow::BorrowManager;
using borrow::Date;
using borrow::Result;
using borrow::ReturnResult;
using borrow::Student;
using borrow::User;

// 可执行文件所在目录（用于定位默认 data 目录）
std::filesystem::path executable_dir() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0) return std::filesystem::path(buffer).parent_path();
#else
    return std::filesystem::current_path();
#endif
    return std::filesystem::current_path();
}

// 数据目录：命令行第一个非选项参数优先；否则用可执行文件旁的 data/（不存在则当前目录 data/）
std::filesystem::path resolve_data_dir(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (!arg.empty() && arg.front() != '-') return arg;
    }
    const auto beside_exe = executable_dir() / "data";
    std::error_code ec;
    if (std::filesystem::exists(beside_exe, ec)) return beside_exe;
    return std::filesystem::current_path() / "data";
}

void print_header(const std::string& title) {
    std::cout << "\n" << std::string(78, '-') << "\n  " << title << "\n"
              << std::string(78, '-') << "\n";
}

void show_result(const Result& result) {
    std::cout << (result.ok ? "  [成功] " : "  [失败] ") << result.message << '\n';
}

void show_return(const ReturnResult& result) {
    std::cout << (result.ok ? "  [成功] " : "  [失败] ") << result.message << '\n';
}

// ---------------------------------------------------------------------------
//  --demo 模式：把 7 个典型场景跑一遍，方便截图写实验报告
//  说明：演示会修改演示数据；需要干净数据时可在菜单"系统设置 -> 重置为演示数据"
// ---------------------------------------------------------------------------
void run_demo(BorrowManager& manager) {
    const Date today = manager.today();
    std::cout << std::format("  系统日期：{}   用户 {} 人   图书 {} 种\n", today.to_string(),
                             manager.list_users().size(), manager.list_books().size());

    print_header("场景1 学生借书成功（学生借期 30 天）");
    if (User* user = manager.find_user("2023001")) {
        if (Book* book = manager.find_book("978-7-111-49997-9")) {
            std::cout << "  " << *user << '\n';
            show_result(manager.borrow_book(*user, *book, today));
        }
    }

    print_header("场景2 学生达到最大借阅数量（新学生上限设为 2 本）");
    manager.add_student("2023999", "演示学生", "计算机学院", "软件工程", "2023级", 2);
    if (User* user = manager.find_user("2023999")) {
        if (Book* first = manager.find_book("978-7-111-40701-4")) {
            show_result(manager.borrow_book(*user, *first, today));
        }
        if (Book* second = manager.find_book("978-7-302-53112-0")) {
            show_result(manager.borrow_book(*user, *second, today));
        }
        if (Book* third = manager.find_book("978-7-111-49997-9")) {
            show_result(manager.borrow_book(*user, *third, today));  // 第 3 本 -> 超限
        }
    }

    print_header("场景3 学生重复借阅同一本书");
    if (User* user = manager.find_user("2023999")) {
        if (Book* book = manager.find_book("978-7-111-40701-4")) {
            show_result(manager.borrow_book(*user, *book, today));
        }
    }

    print_header("场景4 图书已全部借出（《深入理解计算机系统》仅 1 册，已被李四借走）");
    if (User* user = manager.find_user("2022015")) {
        if (Book* book = manager.find_book("978-7-121-35685-4")) {
            std::cout << "  " << *book << '\n';
            show_result(manager.borrow_book(*user, *book, today));
        }
    }

    print_header("场景5 教师借书（教师借期 60 天，与学生的 30 天不同）");
    if (User* user = manager.find_user("T1001")) {
        if (Book* book = manager.find_book("978-7-302-53112-0")) {
            std::cout << "  " << *user << '\n';
            show_result(manager.borrow_book(*user, *book, today));
        }
    }

    print_header("场景6 逾期归还（学生李四，逾期 10 天，0.20 元/天）");
    if (User* user = manager.find_user("2023002")) {
        const std::string isbn = "978-7-121-35685-4";
        std::cout << std::format("  归还前预览：逾期 {} 天，罚款 {:.2f} 元\n",
                                 manager.overdue_days(*user, isbn, today),
                                 manager.preview_fine(*user, isbn, today));
        show_return(manager.return_book(*user, isbn, today));
    }

    print_header("场景7 归还未借阅的图书");
    if (User* user = manager.find_user("2022015")) {
        show_return(manager.return_book(*user, "978-7-302-53112-0", today));
    }

    print_header("演示结束：统计信息");
    int active = 0;
    for (const auto& record : manager.records().all()) {
        if (!record.returned()) ++active;
    }
    std::cout << std::format("  用户 {} 人，图书 {} 种，借阅记录 {} 条（在借 {} 条）\n",
                             manager.list_users().size(), manager.list_books().size(),
                             manager.records().size(), active);
    for (const User* user : manager.list_users()) {
        std::cout << std::format("  {} {}（{}）已借 {}/{} 本，累计罚款 {:.2f} 元\n", user->id(),
                                 user->name(), user->role_name(), user->borrowed_count(),
                                 user->max_borrow(), user->total_fine());
    }
}

}  // namespace

int main(int argc, char** argv) {
#if defined(_WIN32)
    system("chcp 65001 > nul");
#endif

    try {
        const std::filesystem::path data_dir = resolve_data_dir(argc, argv);
        BorrowManager manager{data_dir};
        std::cout << "数据目录：" << data_dir.string() << '\n';

        if (manager.seed_sample_data()) {
            std::cout << "首次运行：写入演示数据（学生 3 人 / 教师 2 人 / 图书 4 种 / 借阅记录 3 条）\n";
        }

        bool demo_mode = false;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--demo") demo_mode = true;
        }

        if (demo_mode) {
            run_demo(manager);
            return 0;
        }

        borrow::Menu menu{manager};
        menu.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[严重错误] " << e.what() << '\n';
        return 1;
    }
}
