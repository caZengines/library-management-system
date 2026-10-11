// ===========================================================================
//  student.cpp  ——  学生类实现
// ===========================================================================
#include "student.h"

#include "model/book.h"

#include <algorithm>
#include <format>

namespace borrow {

// ---------------------------------------------------------------------------
//  判断学生是否已经借过这本书（用 ISBN 比对）
// ---------------------------------------------------------------------------
bool Student::has_borrowed(const std::string& isbn) const noexcept {
    // 通过cpp20提供的ranges来安全高效查询
    return std::ranges::any_of(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
}

// ---------------------------------------------------------------------------
//  登记一条借阅记录
// ---------------------------------------------------------------------------
void Student::add_borrowed_item(Book* book, const Date& borrow_date, const Date& due_date) {
    // 检查空指针避免意外
    if (book == nullptr) throw std::invalid_argument("借阅记录中的图书不能为空");
    borrowed_.emplace_back(BorrowItem{book, borrow_date, due_date});
}

// ---------------------------------------------------------------------------
//  归还时从借阅清单中移除对应记录
// ---------------------------------------------------------------------------
bool Student::remove_borrowed_item(const std::string& isbn) {
    // 由于对stl容器进行删除操作会使数据重排进而导致迭代器失效，
    // 所以我这里用erase_if直接获取删除数量来判断是否成功删除
    const auto removed = std::erase_if(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
    return removed > 0;
}

const Student::BorrowItem* Student::find_item(const std::string& isbn) const {
    const auto it = std::ranges::find_if(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
    return it == borrowed_.end() ? nullptr : &*it;
}

std::string Student::to_string() const {
    return std::format("{:<10} {:<10} {:<16} {:>6} {:>8} {:>10.2f}",
                       id_, name_, major_, max_borrow_, borrowed_count(), total_fine_);
}

}  // namespace borrow
