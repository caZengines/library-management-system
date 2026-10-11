// ===========================================================================
//  book.cpp  ——  图书类实现
// ===========================================================================
#include "book.h"

#include <format>
#include <stdexcept>

namespace borrow {

std::string Book::status_text() const {
    return borrowable() ? std::format("可借(在馆{}/{}册)", available_, total_)
                        : std::format("不可借(已借完 0/{}册)", total_);
}

// ---------------------------------------------------------------------------
//  借出一册：在馆册数减 1，累计借出次数加 1
// ---------------------------------------------------------------------------
bool Book::take_one() {
    //判断是否可借出，不可借出返回false
    if (!borrowable()) return false;
    --available_;
    ++borrow_count_;
    return true;
}

// ---------------------------------------------------------------------------
//  归还一册：在馆册数加 1
// ---------------------------------------------------------------------------
bool Book::give_one() {
    //先判断在馆数量是否已经满足注册总数，超出说明数据异常
    if (available_ >= total_) return false;
    ++available_;
    return true;
}

// ---------------------------------------------------------------------------
//  补充馆藏
// ---------------------------------------------------------------------------
void Book::add_copies(int count) {
    //确保补充数量大于0
    if (count <= 0) throw std::invalid_argument("补充馆藏数量必须为正数");
    total_ += count;
    available_ += count;
}

std::string Book::to_string() const {
    return std::format("{:<16} {:<28} {:<12} {:>6} {:>8} {:>8}",
                       isbn_, title_, author_, total_, available_, borrow_count_);
}

}  // namespace borrow
