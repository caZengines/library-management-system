// ===========================================================================
//  menu.h  ——  控制台菜单界面
// ===========================================================================
#pragma once

#include <string>

namespace borrow {

class BorrowManager;

class Menu {
public:
    explicit Menu(BorrowManager& manager) : manager_(manager) {}

    void run();  // 主循环：显示菜单 -> 分发 -> 处理

private:
    // ---- 各级菜单 ----
    void print_main_menu() const;
    void handle_users();
    void handle_books();
    void handle_borrow();
    void handle_reports();
    void handle_system();

    // ---- 用户管理 ----
    void show_users();
    void add_user();
    void edit_user();
    void delete_user();
    void search_users();

    // ---- 图书管理 ----
    void show_books();
    void add_book();
    void edit_book();
    void delete_book();
    void search_books();

    // ---- 借书 / 还书 ----
    void borrow_book();
    void return_book();
    void show_active_records();

    // ---- 查询统计 ----
    void show_all_records();
    void show_overdue_report();
    void show_user_detail();
    void show_summary();

    BorrowManager& manager_;
};

}  // namespace borrow
