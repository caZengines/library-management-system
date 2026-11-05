// ===========================================================================
//  repository.h  ——  泛型内存仓储（类模板 + 概念 + 主键策略）
//  ---------------------------------------------------------------------------
//       1) 类模板 Repository<T, KeyOf>：同一份"增删改查、条件筛选"代码
//          同时用于 Book（值类型）、BorrowRecord（值类型）和
//          std::unique_ptr<User>（多态指针类型），避免为每种实体重复实现容器逻辑；
//       2) 主键策略 KeyOf：值类型用 DirectKey（item.id()），
//          多态指针用 PointerKey（item->id()），用模板参数把差异"参数化"；
//       3) C++20 concept Keyed<T, KeyOf>：在编译期约束"能被仓储管理的类型
//          必须在该策略下提供字符串主键"，接口用错会直接编译报错；
//       4) 成员函数模板 where(Predicate)：条件筛选，谓词类型由调用方推断。
// ===========================================================================
#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace borrow {

// ---------------------------------------------------------------------------
//  主键策略（Policy）
// ---------------------------------------------------------------------------
struct DirectKey {
    template <typename T>
    static const std::string& get(const T& value) {
        return value.id();
    }
};

struct PointerKey {
    template <typename T>
    static const std::string& get(const T& value) {
        return value->id();
    }
};

// ---------------------------------------------------------------------------
//  概念：在 KeyOf 策略下，T 必须能取出 const std::string& 形式的主键
// ---------------------------------------------------------------------------
template <typename T, typename KeyOf>
concept Keyed = requires(const T& value) {
    { KeyOf::get(value) } -> std::convertible_to<const std::string&>;
};

// ---------------------------------------------------------------------------
//  类模板：Repository
// ---------------------------------------------------------------------------
template <typename T, typename KeyOf = DirectKey>
    requires Keyed<T, KeyOf>
class Repository {
public:
    using value_type = T;

    [[nodiscard]] const std::vector<T>& all() const noexcept { return items_; }
    [[nodiscard]] bool empty() const noexcept { return items_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }

    // ---------------------------- 查询 ----------------------------
    [[nodiscard]] const T* find(const std::string& key) const {
        const auto it = std::ranges::find_if(items_, [&key](const T& item) {
            return KeyOf::get(item) == key;
        });
        return it == items_.end() ? nullptr : &*it;
    }

    [[nodiscard]] T* find(const std::string& key) {
        const auto it = std::ranges::find_if(items_, [&key](const T& item) {
            return KeyOf::get(item) == key;
        });
        return it == items_.end() ? nullptr : &*it;
    }

    [[nodiscard]] bool contains(const std::string& key) const { return find(key) != nullptr; }

    // 条件查找：返回第一个满足谓词的对象（找不到返回 nullptr）
    template <typename Predicate>
    [[nodiscard]] const T* find_if(Predicate predicate) const {
        for (const T& item : items_) {
            if (predicate(item)) return &item;
        }
        return nullptr;
    }

    // 条件筛选：返回所有满足谓词的对象指针
    template <typename Predicate>
    [[nodiscard]] std::vector<const T*> where(Predicate predicate) const {
        std::vector<const T*> result;
        for (const T& item : items_) {
            if (predicate(item)) result.push_back(&item);
        }
        return result;
    }

    // ---------------------------- 增删改 ----------------------------
    T& add(T value) {
        const std::string key = KeyOf::get(value);  // 先取键，避免 move 之后访问
        if (contains(key)) {
            throw std::invalid_argument(std::format("主键已存在：{}", key));
        }
        items_.push_back(std::move(value));
        return items_.back();
    }

    void update(T value) {
        const std::string key = KeyOf::get(value);
        for (T& item : items_) {
            if (KeyOf::get(item) == key) {
                item = std::move(value);
                return;
            }
        }
        throw std::invalid_argument(std::format("要修改的对象不存在：{}", key));
    }

    bool remove(const std::string& key) {
        const auto removed = std::erase_if(items_, [&key](const T& item) {
            return KeyOf::get(item) == key;
        });
        return removed > 0;
    }

    void clear() noexcept { items_.clear(); }

private:
    std::vector<T> items_;
};

}  // namespace borrow
