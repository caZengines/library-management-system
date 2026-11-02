// ===========================================================================
//  borrow_record.h  ——  借阅记录（借书流水）
//  ---------------------------------------------------------------------------
//  说明：一条记录 = 谁（用户编号）借了哪本书（ISBN），何时借、应还、实还、罚了多少，
//        未归还的记录会在程序启动时"挂回"对应用户的借阅清单（见 BorrowManager::load）。
// ===========================================================================
#pragma once

#include "util/date.h"

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace borrow {

// 记录状态：借阅中 / 已归还 / 逾期未还（逾期由"当天 > 应还日"计算得出）
enum class RecordStatus { Borrowed, Returned, Overdue };

class BorrowRecord {
public:
    BorrowRecord() = default;
    BorrowRecord(std::string id,
                 std::string user_id,
                 std::string book_isbn,
                 Date borrow_date,
                 Date due_date,
                 Date return_date = {},
                 double fine = 0.0,
                 bool returned = false)
        : id_(std::move(id)),
          user_id_(std::move(user_id)),
          book_isbn_(std::move(book_isbn)),
          borrow_date_(borrow_date),
          due_date_(due_date),
          return_date_(return_date),
          fine_(fine),
          returned_(returned) {}

    // ---------------------------- 基本字段 ----------------------------
    const std::string& id() const noexcept { return id_; }              // 记录号（主键）
    const std::string& user_id() const noexcept { return user_id_; }    // 借阅人编号
    const std::string& book_isbn() const noexcept { return book_isbn_; }  // 图书 ISBN
    const Date& borrow_date() const noexcept { return borrow_date_; }
    const Date& due_date() const noexcept { return due_date_; }
    const Date& return_date() const noexcept { return return_date_; }
    double fine() const noexcept { return fine_; }
    bool returned() const noexcept { return returned_; }

    void set_return_date(Date value) { return_date_ = value; }
    void set_fine(double value) { fine_ = value; }
    void set_returned(bool value) { returned_ = value; }

    // ---------------------------- 状态判定 ----------------------------
    RecordStatus status(const Date& today) const;
    std::string status_text(const Date& today) const;
    long long overdue_days_now(const Date& today) const;  // 未归还时的当前逾期天数

    // ---------------------------- 序列化 ----------------------------
    // 记录号|借阅人|ISBN|借书日|应还日|归还日(-表示未还)|罚款|是否归还(1/0)
    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static std::optional<BorrowRecord> parse(std::string_view line);
    std::string to_string() const;

    // 流运算符
    friend std::ostream& operator<<(std::ostream& out, const BorrowRecord& record);
    friend std::istream& operator>>(std::istream& in, BorrowRecord& record);

private:
    std::string id_;
    std::string user_id_;
    std::string book_isbn_;
    Date borrow_date_{};
    Date due_date_{};
    Date return_date_{};
    double fine_{0.0};
    bool returned_{false};
};

}  // namespace borrow
