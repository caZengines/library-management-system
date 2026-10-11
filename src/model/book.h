// ===========================================================================
//  book.h  ——  图书类
//  ---------------------------------------------------------------------------
//  题目要求：抽象出图书的成员变量与基本操作；书的"可借 / 不可借"状态用
//            【在馆册数 available_ 与 总册数 total_】表达，
//            并提供状态查询接口供借书逻辑判断。
// ===========================================================================
#pragma once

#include <string>

namespace borrow {

class Book {
public:
    Book() = default;
    Book(std::string isbn, std::string title, std::string author,
         int total = 1, int available = 1)
        : isbn_(std::move(isbn)),
          title_(std::move(title)),
          author_(std::move(author)),
          total_(total),
          available_(available) {}

    // ---------------------------- 基本信息 ----------------------------
    const std::string& isbn() const noexcept { return isbn_; }
    const std::string& title() const noexcept { return title_; }
    const std::string& author() const noexcept { return author_; }
    int total() const noexcept { return total_; }
    int available() const noexcept { return available_; }
    int borrow_count() const noexcept { return borrow_count_; }

    void set_title(std::string title) { title_ = std::move(title); }
    void set_author(std::string author) { author_ = std::move(author); }
    void set_total(int total) { total_ = total; }
    void set_available(int available) { available_ = available; }
    void set_borrow_count(int count) { borrow_count_ = count; }

    // ---------------------------- 状态查询 ----------------------------
    // 是否可借：在馆册数 > 0
    bool borrowable() const noexcept { return available_ > 0; }
    std::string status_text() const;

    // ---------------------------- 状态修改 ----------------------------
    // 借出：在馆册数 -1，借阅次数 +1（由借书逻辑在"状态允许"后调用）
    bool take_one();
    // 归还：在馆册数 +1（不允许超过总册数）
    bool give_one();
    // 补充馆藏
    void add_copies(int count);

    // 输出一行信息
    std::string to_string() const;

private:
    std::string isbn_;          // 图书号
    std::string title_;         // 书名
    std::string author_;        // 作者
    int total_{1};              // 馆藏总册数
    int available_{1};          // 当前在馆（可借）册数
    int borrow_count_{0};       // 累计借出次数
};

}  // namespace borrow
