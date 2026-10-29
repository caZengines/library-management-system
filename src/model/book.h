// ===========================================================================
//  book.h  ——  图书类
//  ---------------------------------------------------------------------------
//  实验归属：
//   【实验3·继承与派生】Book 作为独立的"被借阅对象"，与 User 体系协作；
//                       借阅记录中用 Book* 表示"学生 → 图书"的关系。
//   【实验6·输入输出流】operator<< / operator>> 重载，
//                       以及 serialize()/parse() 供文件流持久化使用。
// ===========================================================================
#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

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
    // id() 即ISBN码
    const std::string& id() const noexcept { return isbn_; }
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
    bool borrowable() const noexcept { return available_ > 0; }
    std::string status_text() const;

    // ---------------------------- 状态修改 ----------------------------
    bool take_one();                 // 借出：在馆册数 -1，借出次数 +1
    bool give_one();                 // 归还：在馆册数 +1
    void add_copies(int count);      // 补充馆藏

    // ---------------------------- 序列化 ----------------------------
    // 一行文本：ISBN|书名|作者|总册数|在馆册数|借出次数
    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static std::optional<Book> parse(std::string_view line);
    std::string to_string() const;

    // 【实验6·输入输出流】流运算符重载
    friend std::ostream& operator<<(std::ostream& out, const Book& book);
    friend std::istream& operator>>(std::istream& in, Book& book);

private:
    std::string isbn_;          // 图书号（主键）
    std::string title_;         // 书名
    std::string author_;        // 作者
    int total_{1};              // 馆藏总册数
    int available_{1};          // 当前在馆（可借）册数
    int borrow_count_{0};       // 累计借出次数
};

}  // namespace borrow
