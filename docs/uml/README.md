# UML 图 —— 学生借书系统

本目录存放《组合、依赖关系》实验的 UML 图（PlantUML 源文件 + 已渲染的 PNG/SVG）。

| 源文件 | 图名 | 报告中的编号 | 用途 |
|---|---|---|---|
| `borrow-class-diagram.puml` | 类图 | 图 3-1 | 类名/属性/方法/类间关系，重点表达**组合关系**与**依赖关系** |
| `borrow-flowchart.puml` | 核心函数流程图 | 图 3-2 | `BorrowManager::borrow_book()` 的执行流程 |
| `core-flow.puml` | 借还时序图 | 附录 | 借书四项校验与还书逾期罚款的逐步消息流 |

渲染产物在 `img/` 下，同名 `.png`（插入文档用）与 `.svg`（放大不失真用）。

---

## 一、borrow-class-diagram —— 类图

- **组合关系（实心菱形 ◆）**：`BorrowManager *-- "1..*" Book` 与
  `BorrowManager *-- "1..*" Student`；管理者用 `std::vector<Book>`、
  `std::vector<Student>` **按值**持有对象，对象随管理者创建而创建、随管理者析构而析构。
  另有一层嵌套组合：`Student *-- "0..*" BorrowItem`。
- **依赖关系（虚线箭头 ┄>）**：`BorrowManager ..> Book`、`BorrowManager ..> Student`
  表示借书/还书动作以**引用**接收对象；`BorrowItem --> Book`（`Book*` 字段）
  表示学生只"使用"图书而**不拥有**它，因此不负责 `delete`。
- 另有 `Date`（值类型）、`Result` / `ReturnResult`（返回值类型）作为依赖对象。

## 二、borrow-flowchart —— 核心函数流程图

`BorrowManager::borrow_book(Student&, Book&, const Date&)` 四步：

1. 学生状态校验：`can_borrow_more()`、`has_borrowed(book.isbn())`；
2. 图书状态校验：`book.borrowable()`；
3. 两者都允许才同步改变双方状态：`book.take_one()` + `student.add_borrowed_item(&book, ...)`；
4. 返回 `Result`。

任一校验不通过立即返回失败，且**不修改任何状态**。

## 三、core-flow —— 借还时序图

- 借书场景：`Cli → BorrowService::borrow_book` 的四项校验与应还日计算（本图为完整版系统绘制，
  与 `borrow_book` 的校验逻辑一一对应）；
- 还书场景：`preview_fine()` 预览罚款 → `return_book()` 结算并回写状态。

---

## 重新渲染

本机已带 Java 25 与 `plantuml-1.2026.8.jar`（仓库根目录，未入库）：

```powershell
# 注意：PowerShell 通配符对含中文名的 .puml 匹配不稳，建议逐个文件传入
$env:JAVA_TOOL_OPTIONS = '-Djava.awt.headless=true'
$files = 'docs/uml/borrow-class-diagram.puml','docs/uml/borrow-flowchart.puml','docs/uml/core-flow.puml'
foreach ($fmt in 'svg','png') {
  foreach ($f in $files) {
    java -jar plantuml-1.2026.8.jar -Playout=smetana -charset UTF-8 "-t$fmt" -o img $f
  }
}
```

说明：

- `-Playout=smetana` 使用 PlantUML 内置布局引擎，**无需安装 Graphviz**；装了 Graphviz
  时去掉该参数即用 `dot` 布局（线条更规整）。
- `-charset UTF-8` 保证中文标题与注释正常显示。
- PlantUML 按图内 `@startuml <名称>` 命名产物，生成后把 `img/` 下的中文名文件改成
  与 `.puml` 同名的英文名即可（仓库内已规范化）。
- 类名与方法名与 `src/` 下代码一一对应，改代码后重跑上面的命令即可同步。
