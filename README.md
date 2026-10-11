# 面向对象程序设计（C++）—— 学生借书系统

本项目按"一步一步搭建系统"的方式推进，当前阶段完成 **《组合、依赖关系》** 实验：
对学生和图书进行抽象，学生发起借书操作（依赖关系），借阅成功时学生与图书的状态同步改变。

- 语言标准：**C++20**（`std::format` / `std::ranges` / `std::erase_if`）
- 构建工具：CMake ≥ 3.20（GCC 13+ / Clang 16+ / MSVC 19.30+）
- 第三方依赖：无（全部使用标准库）
- 数据存储：内存存储；`BorrowManager` 已提供 `load()/save()` 文本文件读写接口，后续阶段接入

## 一、类间关系（本阶段重点）

### 1. 组合关系（Composition）
`BorrowManager` 用**按值的容器**持有学生与图书，生命周期完全由管理者控制：

```cpp
class BorrowManager {
private:
    std::vector<Book>    books_;      // 组合：图书由管理者拥有
    std::vector<Student> students_;   // 组合：学生由管理者拥有
};
```
学生内部还有一层嵌套组合：`Student` 按值持有 `std::vector<BorrowItem>`。

### 2. 依赖关系（Dependency）
借书动作用**引用**接收对象，并只保存 `Book*` 而不负责其生死：

```cpp
Result borrow_book(Student& student, Book& book, const Date& date);  // 引用传参
...
student.add_borrowed_item(&book, date, due_date);                    // 只借用，不销毁
```
图书的所有权在管理者手里，因此 `BorrowItem::book` 是裸指针且**不 delete**，避免双重释放。

### 3. 借书成功的判定与状态同步
| 校验对象 | 判据 | 不通过时的提示 |
|---|---|---|
| 学生状态 | `can_borrow_more()`：已借数量 < 最大借阅数量 | 已达最大借阅数量 |
| 学生状态 | `has_borrowed(isbn)`：不能重复借同一本 | 不能重复借同一本书 |
| 图书状态 | `borrowable()`：在馆册数 > 0 | 图书已全部借出 |

两者都允许后才同步变更：`book.take_one()`（在馆册数 −1、借出次数 +1）
+ `student.add_borrowed_item(&book, date, due)`（借阅清单 +1）。

### 4. 逾期罚款
还书时按"应还日 vs 实际归还日"计算：`罚款 = 逾期天数 × 0.20 元/天`，
逾期天数通过把日期换算成"距 1970-01-01 的天数"后相减得到（自实现，避免依赖 `<chrono>` 日历部分）。

## 二、目录结构

```
library-management/
├─ CMakeLists.txt              # 构建脚本（目标 borrow_demo，C++20）
├─ README.md                   # 本文件
├─ 组合依赖关系_设计报告.docx    # 实验设计报告
├─ requires.txt                # 实验题目与要求
├─ docs/uml/                   # UML 图（PlantUML 源文件 + PNG/SVG 渲染产物）
│  ├─ borrow-class-diagram.*   # 类图（组合关系 + 依赖关系）
│  ├─ borrow-flowchart.*       # 核心函数 borrow_book 流程图
│  └─ core-flow.*              # 借书/还书时序图
└─ src/
   ├─ main.cpp                 # 演示程序：7 个测试场景
   ├─ model/
   │  ├─ book.h / book.cpp         # 图书类：ISBN/书名/作者/总册数/在馆册数/借出次数
   │  └─ student.h / student.cpp   # 学生类：学号/姓名/专业/最大借阅数量/借阅清单/累计罚款
   ├─ service/
   │  └─ borrow_manager.h / .cpp   # 借阅管理者：组合持有 + 借书还书（依赖）
   └─ util/
      └─ date.h / date.cpp         # 日期工具：借书日 / 应还日 / 逾期天数
```

## 三、构建与运行

```bash
# CMake（推荐）
cmake -S . -B build && cmake --build build
./build/borrow_demo            # Windows: build\borrow_demo.exe

# 或者直接编译
g++ -std=c++20 -Wall -Wextra -Isrc src/main.cpp src/model/*.cpp src/service/*.cpp src/util/*.cpp -o borrow_demo
```

演示程序依次输出 7 个场景：正常借书成功、达到最大借阅数量、重复借同一本书、
图书已全部借出、按期归还、逾期归还（15 天 × 0.20 = 3.00 元）、归还未借图书。

## 四、需要自行补充实现的位置

题目要求"核心函数算法代码需包含自己的修改注释"，因此以下位置预留了标记
（在编辑器中搜索 `【请自行实现】` 即可定位），未填写时程序会返回失败提示，属正常现象：

| 编号 | 文件 | 函数 | 内容 |
|---|---|---|---|
| (1/6) | `src/model/book.cpp` | `Book::take_one()` | 借出：在馆册数 −1、借出次数 +1 |
| (2/6) | `src/model/book.cpp` | `Book::give_one()` | 归还：在馆册数 +1 |
| (3/6) | `src/model/book.cpp` | `Book::add_copies()` | 补充馆藏（参数校验） |
| (4/6) | `src/model/student.cpp` | `Student::has_borrowed()` | 用 STL 算法判断是否已借该书 |
| (5/6) | `src/model/student.cpp` | `Student::add_borrowed_item()` | 借阅清单新增 |
| (6/6) | `src/model/student.cpp` | `Student::remove_borrowed_item()` | 借阅清单删除（迭代器失效） |
| (7/7) | `src/service/borrow_manager.cpp` | `BorrowManager::borrow_book()` | **借书主流程**（报告重点） |

异常处理预留 4 处标记（搜索 `【异常处理·占位符】`）：
`borrow_manager.cpp` 的 `load()`/`save()`（数据文件容错与写入失败）、
`main.cpp` 的业务异常分层捕获与最外层兜底。

> 说明：`borrow_book()` 中另附一份**已注释的参考实现**，用于自己写完代码后对比思路；
> 提交前建议删除该注释块，以符合"包含自己的修改注释"的要求。

## 五、后续阶段规划

1. **阶段二 · 数据持久化**：把 `load()/save()` 接入主流程，新增借阅记录文件，保证重启后状态可恢复。
2. **阶段三 · 界面层**：增加菜单式控制台交互（图书入库、学生注册、借书、还书、查询统计）。
3. **阶段四 · 关系扩展**：引入读者基类 `Reader`（学生/教师/校外读者），
   用继承 + 多态区分借期与罚款策略，用组合持有 `BorrowPolicy` 策略对象。
