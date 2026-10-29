// ===========================================================================
//  book.cpp  ——  图书类实现（含文件序列化与流运算符）
// ===========================================================================
#include "book.h"

#include "util/strings.h"

#include <format>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace borrow {
namespace {

// 把整数字段解析为 int，失败返回 nullopt（文件可能被手工改坏）
std::optional<int> to_int(const std::string& text) {
    try {
        std::size_t used = 0;
        const int value = std::stoi(text, &used);
        if (used != text.size()) return std::nullopt;
        return value;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

std::string Book::status_text() const {
    return borrowable() ? std::format("可借(在馆{}/{}册)", available_, total_)
                        : std::format("不可借(已借完 0/{}册)", total_);
}

// ---------------------------------------------------------------------------
//  借出一册：在馆册数减 1，累计借出次数加 1
// ---------------------------------------------------------------------------
bool Book::take_one() {
    if (!borrowable()) return false;
    --available_;
    ++borrow_count_;
    return true;
}

// ---------------------------------------------------------------------------
//  归还一册：在馆册数加 1（不允许超过总册数）
// ---------------------------------------------------------------------------
bool Book::give_one() {
    if (available_ >= total_) return false;
    ++available_;
    return true;
}

// ---------------------------------------------------------------------------
//  补充馆藏
// ---------------------------------------------------------------------------
void Book::add_copies(int count) {
    if (count <= 0) throw std::invalid_argument("补充馆藏数量必须为正数");
    total_ += count;
    available_ += count;
}

// ---------------------------------------------------------------------------
//  【实验6·输入输出流】序列化：把对象写成一行的文本表示
//  文件持久化时由 std::ofstream << serialize() 输出，读取时用 parse() 还原。
// ---------------------------------------------------------------------------
std::string Book::serialize() const {
    return std::format("{}|{}|{}|{}|{}|{}", isbn_, title_, author_, total_, available_,
                       borrow_count_);
}

std::optional<Book> Book::parse(std::string_view line) {
    const auto fields = split(line, '|');
    if (fields.size() != 6) return std::nullopt;

    const auto total = to_int(fields[3]);
    const auto available = to_int(fields[4]);
    const auto borrow_count = to_int(fields[5]);
    if (!total || !available || !borrow_count) return std::nullopt;

    Book book{fields[0], fields[1], fields[2], *total, *available};
    book.set_borrow_count(*borrow_count);
    return book;
}

std::string Book::to_string() const {
    return std::format("{:<16} {:<28} {:<14} {:>5} {:>6} {:>6}",
                       isbn_, title_, author_, total_, available_, borrow_count_);
}

// ===========================================================================
//  【实验6·输入输出流】Book 的流运算符重载
//  ---------------------------------------------------------------------------
//  operator<<：输出一行人类可读的图书摘要（可写 cout / ofstream / ostringstream）
//  operator>>：从输入流中依次读取"非注释、非空"的数据行，
//              交给 parse() 还原对象；遇到损坏行自动跳过，读到结尾置 failbit。
// ===========================================================================
std::ostream& operator<<(std::ostream& out, const Book& book) {
    return out << std::format("《{}》 {} | ISBN {} | {}", book.title_, book.author_, book.isbn_,
                              book.status_text());
}

std::istream& operator>>(std::istream& in, Book& book) {
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // 兼容 Windows 换行
        if (line.empty() || line.front() == '#') continue;          // 跳过注释与空行
        if (auto parsed = Book::parse(line)) {
            book = std::move(*parsed);
            return in;
        }
        // 该行损坏：跳过并继续读取下一行
    }
    in.setstate(std::ios::failbit);  // 到达文件末尾/无有效数据
    return in;
}

}  // namespace borrow
