# 逾期费用与借阅期限 设计文档

日期：2026-06-30
状态：已确认，待实现

## 背景

大作业要求中有两项尚未实现，且都是评分点：

- **要求 (4)** 明确列出"逾期费用计算"，当前代码完全没有。
- **要求 (2)** 要求不同读者具有"不同的最长借书期限"和"不同的超期收费标准"，当前 `StudentReader`/`TeacherReader` 只在借阅**数量**上限（5 vs 20）上有差异。
- **要求 (1)** 举例提到图书的"借出最大期限"，当前 `Book` 及子类没有相关属性。

这三处是同一条主线：**日期 → 借期 → 逾期 → 罚款**。现有架构（多态接口 + `BorrowingRecord` 已记录借/还日期）很适合在其上扩展。

## 目标

1. 图书具备"借出最大期限"属性，且按图书类型多态化。
2. 读者具备"最长借书期限"和"每日超期罚款标准"，且按读者类型多态化。
3. 提供逾期费用计算，接入还书流程与菜单查询。
4. 不破坏现有 CSV 持久化格式、不修改现有任何测试、不修改 `returnBook` 签名。

## 非目标（YAGNI）

- 不把罚款金额落盘到 `records.csv`（按需计算即可）。
- 不引入第三方日期库。
- 不做罚款缴纳/账户余额等账务功能。

## 设计决策（已与用户确认）

- **借期取两者较小值**：一本书的有效借期 = `min(图书借期, 读者借期)`，两个继承体系都用上多态，同时满足要求 (1) 和 (2)。
- **罚款按需计算，CSV 格式不变**：由 `BorrowingRecord` 的 `borrowDate` + 读者/图书期限 + 传入的 `asOfDate` 现场算出，不持久化。

## 组件设计

### 1. `DateUtil`（新增 `include/DateUtil.h` + `src/DateUtil.cpp`）

独立、可单独测试的日期工具，只负责字符串日期的算术，业务代码不直接解析日期字符串。

```cpp
namespace library {
    // 解析 "YYYY-MM-DD"，返回 to - from 的天数差（to 晚于 from 时为正）。
    // 采用 days-from-civil 算法，避开 mktime 的时区/夏令时问题。
    int daysBetween(const std::string& from, const std::string& to);
}
```

- 内部用 Howard Hinnant 的 `days_from_civil` 算法把 (year, month, day) 换算成自纪元的天数序号，两者相减。
- 输入格式固定为 `YYYY-MM-DD`（与系统其它部分一致）。

### 2. 图书侧多态：`Book::maxBorrowDays()`（要求 1）

`Book` 基类新增纯虚接口：

```cpp
[[nodiscard]] virtual int maxBorrowDays() const = 0;
```

**约定：返回值 `< 0` 表示"不计逾期 / 不外借"。**

| 类型 | `maxBorrowDays()` | 含义 |
|------|------------------|------|
| `PhysicalBook` | `30` | 纸质书借期 30 天 |
| `EBook` | `-1` | 电子书可无限持有，永不逾期 |
| `Magazine` | `-1` | 不外借（`isBorrowable()` 已为 false）|

各子类 `availabilityInfo()` 顺带展示借期信息（纸质书展示"借期 30 天"，电子书/杂志说明不计逾期/不外借）。

### 3. 读者侧多态：`Reader::maxBorrowDays()` + `Reader::finePerDay()`（要求 2）

`Reader` 基类新增两个纯虚接口：

```cpp
[[nodiscard]] virtual int maxBorrowDays() const = 0;    // 最长借书期限（天）
[[nodiscard]] virtual double finePerDay() const = 0;    // 每日超期罚款（元）
```

| 类型 | `maxBorrowDays()` | `finePerDay()` |
|------|------------------|----------------|
| `StudentReader` | `30` 天 | `0.50` 元/天 |
| `TeacherReader` | `60` 天 | `0.20` 元/天 |

设计意图：教师借期更长、日罚更低（待遇更好），数值可调，关键在于体现类型差异。

`Reader::display()` 顺带展示借期与日罚标准。

### 4. `LibrarySystem::calculateFine`

```cpp
// 计算某读者所借某书截至 asOfDate 的逾期费用（元）。
// 找不到读者/图书 → 抛 ReaderNotFoundException / BookNotFoundException（与现有风格一致）。
[[nodiscard]] double calculateFine(const std::string& readerId,
                                   const std::string& bookId,
                                   const std::string& asOfDate) const;
```

算法：

1. `findReader` / `findBook`，找不到抛对应异常。
2. 在 `records_` 中找该读者 + 该书 **未归还**（`!isReturned()`）的记录，取其 `borrowDate`；找不到这样的记录 → 返回 `0.0`。
3. 若 `book->maxBorrowDays() < 0` → 返回 `0.0`（电子书/杂志永不逾期，体现多态差异）。
4. `int dueDays = std::min(book->maxBorrowDays(), reader->maxBorrowDays());`
5. `int overdue = daysBetween(borrowDate, asOfDate) - dueDays;`
6. `if (overdue <= 0) return 0.0;`
7. `return overdue * reader->finePerDay();`

配套：将 `LibrarySystem::today()` 由 `private` 改为 `public static`，供菜单获取当天日期；测试中传入固定日期，不依赖系统时钟。

### 5. 接入还书与菜单（要求 4）

- `returnBook` **签名保持不变**。在 `main.cpp` 的 `doReturn` 中：还书成功后调用 `calculateFine(readerId, bookId, today())`（注意：还书会把记录标记为已归还，因此在 `returnBook` 调用**之前**先记下当天日期并查询逾期记录——见实现说明），若 > 0 则打印逾期费用。
  - 实现说明：由于 `calculateFine` 依赖"未归还"的记录，而 `returnBook` 会将其标记为已归还，故在 `doReturn` 中**先**调用 `calculateFine(..., today())` 拿到费用，**再**调用 `returnBook`；或者由 `returnBook` 内部在标记归还前算好费用。为保持 `returnBook` 签名不变且逻辑清晰，采用前者：先查后还。
- 菜单新增 **"9) 逾期费用查询"**：输入读者 ID + 图书 ID，对其在借图书按当天 (`today()`) 计算并展示逾期费用（0 元时提示未逾期）。
- 菜单选项 `9` 插入在 `8) 保存数据` 之后、`0) 保存并退出` 之前，`printMenu` 与 `switch` 同步更新。

### 6. 测试（新增 `test/test_fine.cpp`，Makefile 自动收录）

采用与 `test_library.cpp` 相同的轻量断言风格（无第三方依赖）。覆盖：

- `daysBetween`：跨月、跨年、同日为 0、负数方向。
- `calculateFine` 学生纸质书逾期：构造已知 `borrowDate`，传入晚于到期日的 `asOfDate`，断言罚款 = 逾期天数 × 0.50。
- `calculateFine` 教师纸质书逾期：同上，日罚 0.20，且因借期 60 天 > 学生 30 天而到期更晚。
- 借期取 min：教师借纸质书时有效借期应为 `min(30, 60) = 30`，验证以图书 30 天为准。
- 电子书罚款恒为 0：即便持有很久，`maxBorrowDays() < 0`，罚款为 0。
- 未逾期：`asOfDate` 在到期日内，罚款为 0。

> 注：`calculateFine` 依赖"未归还"记录，测试中通过 `borrowBook` 建立记录后，用 `LibrarySystem` 公开接口构造场景；`borrowDate` 由系统写入当天，测试以"相对当天偏移"的方式构造 `asOfDate`（用 `today()` + `daysBetween` 反推，或直接断言 `calculateFine(today())` 在借期内为 0、远期为正）。具体在实现阶段以可稳定通过为准。

## 影响面

新增文件：
- `include/DateUtil.h`, `src/DateUtil.cpp`
- `test/test_fine.cpp`

修改文件：
- `include/book/Book.h`（+ 纯虚 `maxBorrowDays`）
- `include/book/PhysicalBook.h` / `EBook.h` / `Magazine.h` 及对应 `.cpp`（实现 `maxBorrowDays`，更新 `availabilityInfo`）
- `include/reader/Reader.h`（+ 两个纯虚接口），`src/reader/Reader.cpp`（更新 `display`）
- `include/reader/Student.h` / `Teacher.h` 及对应 `.cpp`（实现两个接口）
- `include/LibrarySystem.h`（`today()` 改 public、+ `calculateFine`），`src/LibrarySystem.cpp`（实现 `calculateFine`）
- `main.cpp`（菜单项 + 还书展示逾期费用）
- `README.md`（补充新功能说明）

**不改动**：CSV 格式与 `CsvStorage`、`BorrowingRecord` 结构、现有全部测试、`returnBook` 签名。

## 验收标准

- `make test` 全部通过（含新增 `test_fine`，且原有测试不变更、不失败）。
- `make` 主程序编译无警告（`-Wall -Wextra`）。
- 菜单"逾期费用查询"与还书流程能正确展示逾期费用。
- 学生/教师在借期和日罚上表现出差异；电子书永不逾期。
