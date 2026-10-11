// ===========================================================================
//  student.cpp  ——  学生类实现
//  注意：标有【请自行实现】的代码块为核心算法，需要你补充自己的实现与注释。
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
    // ===================== 【请自行实现】(4/6) =====================
    // 【核心算法·借书前置校验（学生侧）】
    // TODO(你的实现)：请在这里写自己的代码，并加上你自己的注释说明：
    //   1) 遍历 borrowed_ 清单，比较每个条目的 book->isbn() 与参数 isbn
    //   2) 命中返回 true，否则 false
    //   提示：可以用 std::ranges::any_of 或 std::any_of（体现 STL 算法使用）
    //   我的实现说明：
    // ==============================================================
    return std::ranges::any_of(borrowed_, [&isbn](const BorrowItem& item) {
        return item.book != nullptr && item.book->isbn() == isbn;
    });
}

// ---------------------------------------------------------------------------
//  登记一条借阅记录
// ---------------------------------------------------------------------------
void Student::add_borrowed_item(Book* book, const Date& borrow_date, const Date& due_date) {
    // ===================== 【请自行实现】(5/6) =====================
    // 【核心算法·借阅清单维护：新增】
    // TODO(你的实现)：请在这里写自己的代码，并加上你自己的注释说明：
    //   1) 空指针防御：book == nullptr 时如何处理（抛 std::invalid_argument）
    //   2) 用 push_back / emplace_back 把 {book, borrow_date, due_date} 加入 borrowed_
    //   我的实现说明：
    // ==============================================================
    if (book == nullptr) throw std::invalid_argument("借阅记录中的图书不能为空");
    borrowed_.push_back(BorrowItem{book, borrow_date, due_date});
}

// ---------------------------------------------------------------------------
//  归还时从借阅清单中移除对应记录（STL 容器删除 + 迭代器失效问题）
// ---------------------------------------------------------------------------
bool Student::remove_borrowed_item(const std::string& isbn) {
    // ===================== 【请自行实现】(6/6) =====================
    // 【核心算法·借阅清单维护：删除】
    // TODO(你的实现)：请在这里写自己的代码，并加上你自己的注释说明：
    //   1) 用 std::erase_if（C++20）或"查找 + erase"删除命中 ISBN 的条目
    //   2) 返回是否真的删掉了一条（用于归还逻辑判断）
    //   3) 可在此处说明你如何避免"迭代器失效"问题（报告中"个人收获"可写这点）
    //   我的实现说明：
    // ==============================================================
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
