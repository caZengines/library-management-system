# 面向对象程序设计（C++）—— 图书馆借阅管理系统

本项目由"学生借书"小系统逐步扩展而来：

| 实验 | 主题 | 在本项目中的体现 |
|---|---|---|
| 实验 3 | 继承与派生 | `User` 抽象基类 + `Student` / `Teacher` 公有派生；虚析构、`override`、多态工厂 |
| 实验 4 | 多态性与虚函数 | 借阅上限、借期、罚款单价全部由虚函数给出；基类指针统一管理用户；`dynamic_cast` 区分派生类型 |
| 实验 5 | 模板使用 | 类模板 `Repository<T, KeyOf>` + 主键策略、C++20 `concept` 约束、函数模板 `print_table<T, RowFn>`、成员函数模板 `where(Pred)` |
| 实验 6 | 输入输出流 | `operator<<` / `operator>>` 重载、`ifstream/ofstream` 持久化、`std::getline` 与 `std::from_chars` 输入校验、菜单交互 |

- 语言标准：**C++20**（`std::format` / `std::ranges` / `std::erase_if` / concepts）
- 构建工具：CMake ≥ 3.20（GCC 13+ / Clang 16+ / MSVC 19.30+）
- 第三方依赖：无
- 数据存储：`data/users.txt`、`data/books.txt`、`data/records.txt`（竖线分隔的文本，启动时加载、变更后写回）

## 一、快速开始

```bash
cmake -S . -B build && cmake --build build
./build/library_system            # Windows: build\library_system.exe
```

首次运行会自动写入演示数据（学生 3 人、教师 2 人、图书 4 种、借阅记录 3 条，
其中含 1 条逾期记录，便于演示罚款）。

两种运行方式：

```bash
library_system                 # 菜单交互模式
library_system --demo          # 演示模式：自动跑 7 个典型场景并输出结果
library_system D:/lib_data     # 指定数据目录（不存在会自动创建）
```

> 注意：`--demo` 会真实修改数据。需要恢复初始数据时，可在菜单
> `5. 系统设置 → 6. 重置为演示数据` 中重置。

## 二、目录结构

```
library-management/
├─ CMakeLists.txt
├─ README.md
├─ docs/
│  ├─ EXPERIMENTS.md            # 实验内容 ↔ 代码位置对照表（分批提交 git 用）
│  └─ uml/                      # PlantUML 类图 / 流程图（源文件 + 渲染产物）
└─ src/
   ├─ main.cpp                  # 入口：定位数据目录、演示数据、菜单 / --demo
   ├─ util/
   │  ├─ date.h / .cpp          # 日期运算 + 流运算符重载（实验6）
   │  ├─ strings.h / .cpp       # trim / split / 模糊匹配
   │  ├─ io.h / .cpp            # 控制台输入校验（实验6）
   │  └─ table.h                # 表格打印函数模板 print_table<T>（实验5）
   ├─ model/
   │  ├─ user.h / .cpp          # User 抽象基类、Student、Teacher（实验3/4/6）
   │  ├─ book.h / .cpp          # 图书类 + 序列化 + 流运算符（实验6）
   │  └─ borrow_record.h / .cpp # 借阅记录 + 状态判定 + 序列化（实验6）
   ├─ repository/
   │  └─ repository.h           # 泛型仓储 Repository<T, KeyOf>（实验5）
   ├─ service/
   │  └─ borrow_manager.h / .cpp# 业务核心：用户/图书/借还/罚款/统计
   └─ ui/
      └─ menu.h / .cpp          # 菜单界面（实验6）
```

## 三、核心设计

### 1. 继承与派生（实验 3）

```cpp
class User {                                  // 抽象基类
public:
    virtual ~User() = default;                // 虚析构：基类指针 delete 派生对象
    virtual std::string role_name() const = 0;
    virtual std::string extra_info() const = 0;
    virtual int  max_borrow() const = 0;
    virtual void set_max_borrow(int) = 0;
    virtual int  borrow_days() const = 0;
    virtual double fine_per_day() const = 0;
    virtual std::string serialize() const = 0;
    virtual std::ostream& print(std::ostream&) const;
};

class Student final : public User { /* 专业、年级、上限 5 本、借期 30 天、0.20 元/天 */ };
class Teacher final : public User { /* 职称、     上限 10 本、借期 60 天、0.50 元/天 */ };
```

- 公共属性（编号、姓名、院系、借阅清单、累计罚款）放在基类；
- 专有属性（专业/年级、职称）放在派生类；
- `User::deserialize()` 是**多态工厂**：按行首 `STUDENT` / `TEACHER` 构造对应派生对象，
  因此一个 `users.txt` 能同时保存两类用户。

### 2. 多态性与虚函数（实验 4）

业务层只持有 `User&` / `User*`，不需要知道真实类型：

```cpp
// service/borrow_manager.cpp
if (!user.can_borrow_more()) { ... user.max_borrow() ... }     // 虚函数 -> 学生 5 / 教师 10
const Date due = date.add_days(user.borrow_days());           // 虚函数 -> 学生 30 / 教师 60
const double fine = overdue * user.fine_per_day();            // 虚函数 -> 0.20 / 0.50
```

新增角色（如校外读者）只需增加一个派生类并实现策略，借还算法不用改。

### 3. 模板使用（实验 5）

```cpp
// 主键策略：值对象 / 多态指针各一套
struct DirectKey  { template <typename T> static const std::string& get(const T& v){ return v.id(); } };
struct PointerKey { template <typename T> static const std::string& get(const T& v){ return v->id(); } };

template <typename T, typename KeyOf = DirectKey>
    requires Keyed<T, KeyOf>
class Repository { /* add / find / update / remove / where(pred) */ };

using UserRepository   = Repository<std::unique_ptr<User>, PointerKey>;  // 多态对象
using BookRepository   = Repository<Book>;
using RecordRepository = Repository<BorrowRecord>;
```

同一套增删改查代码复用于三种实体；`where()` 是成员函数模板；
`print_table<T, RowFn>` 是函数模板，配合 `TableEntity` 概念打印任意列表。

### 4. 输入输出流（实验 6）

- **流运算符**：`Date`、`Book`、`BorrowRecord`、`User` 均重载 `operator<<`；
  `Date`、`Book`、`BorrowRecord` 重载 `operator>>`，可直接用于文件读写；
- **文件持久化**：`BorrowManager::load()` 用 `std::ifstream` + `>>` 读取，
  `save()` 用 `std::ofstream` + `<<` 写入；启动时按"图书 → 用户 → 记录"顺序加载，
  再把未归还记录挂回用户的借阅清单；
- **控制台输入**：`util/io` 用 `std::getline` 读整行、`std::from_chars` 解析数字，
  非法输入循环提示并清理流状态；
- **格式化输出**：使用 `std::format` 生成表格与提示。

## 四、业务规则

| 规则 | 说明 |
|---|---|
| 借书前置校验 | ① 未重复借同一本 ② 未超过最大借阅数量 ③ 图书在馆册数 > 0 |
| 应还日期 | 借书日 + 借期（学生 30 天 / 教师 60 天） |
| 逾期判定 | 未归还且"归还日 > 应还日"即逾期；天数 = 归还日 − 应还日 |
| 罚款 | 逾期天数 × 日单价（学生 0.20 元/天，教师 0.50 元/天） |
| 删除保护 | 有未还图书的用户、借出中的图书不允许删除 |
| 数据一致性 | 借书先改图书状态再写记录，任一步失败则回滚；所有变更立即落盘 |
