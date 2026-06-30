# 逾期费用与借阅期限 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 给图书馆系统补上"借出期限 → 逾期 → 罚款"这条主线，满足作业要求 (1)(2)(4) 中缺失的逾期费用计算与差异化期限。

**Architecture:** 新增独立日期工具 `DateUtil` 做天数算术；`Book` 和 `Reader` 两个继承体系各加多态接口（图书借期 / 读者借期 + 日罚）；`LibrarySystem::calculateFine` 现场按 `min(图书借期, 读者借期)` 算逾期，不落盘；菜单与还书流程接入展示。

**Tech Stack:** C++17，g++，手写轻量断言（无第三方依赖），Makefile 自动收录 `src/*.cpp` 与 `test/*.cpp`。

## Global Constraints

- C++17，编译需在 `-Wall -Wextra` 下无警告（见 Makefile）。
- 不改 CSV 格式、不改 `CsvStorage`、不改 `BorrowingRecord` 结构、不改 `returnBook` 签名、不改动现有任何测试文件。
- 日期格式统一 `"YYYY-MM-DD"`。
- 注释少写、要写就偏口语化（向现有代码看齐）。
- 命名空间统一 `library`。
- `Book::maxBorrowDays()` 约定：返回值 `< 0` 表示"不计逾期 / 不外借"。
- 数值：纸质书借期 30 天；学生借期 30 天、日罚 0.50 元；教师借期 60 天、日罚 0.20 元。

---

### Task 1: DateUtil —— 日期天数算术

**Files:**
- Create: `include/DateUtil.h`
- Create: `src/DateUtil.cpp`
- Test: `test/test_fine.cpp`（本任务新建并先放日期相关用例）

**Interfaces:**
- Consumes: 无
- Produces:
  - `int library::daysBetween(const std::string& from, const std::string& to)` —— `to` 晚于 `from` 返回正数天数差。
  - `std::string library::addDays(const std::string& date, int n)` —— 把日期往后推 `n` 天，返回 `"YYYY-MM-DD"`。

- [ ] **Step 1: 写失败测试 `test/test_fine.cpp`**

```cpp
// test/test_fine.cpp
// 逾期费用相关测试，沿用 test_library 的手写断言风格
#include "../include/DateUtil.h"

#include <cmath>
#include <iostream>
#include <string>

using namespace library;

namespace
{
    int g_passed = 0;
    int g_failed = 0;
    const char *g_currentCase = "";

    void beginCase(const char *name)
    {
        g_currentCase = name;
        std::cout << "[ RUN  ] " << name << "\n";
    }
    void endCase() { std::cout << "[ OK   ] " << g_currentCase << "\n"; }
    void reportFail(const char *expr, const char *file, int line)
    {
        ++g_failed;
        std::cout << "[ FAIL ] " << g_currentCase << "\n"
                  << "         " << file << ":" << line << "\n"
                  << "         assertion failed: " << expr << "\n";
    }
    bool approx(double a, double b) { return std::fabs(a - b) < 1e-9; }
}

#define CHECK(cond)                                \
    do                                             \
    {                                              \
        if (cond) { ++g_passed; }                  \
        else { reportFail(#cond, __FILE__, __LINE__); } \
    } while (0)

static void test_days_between()
{
    beginCase("date: daysBetween 基本算术");
    CHECK(daysBetween("2024-01-01", "2024-01-01") == 0);
    CHECK(daysBetween("2024-01-01", "2024-01-11") == 10);
    CHECK(daysBetween("2024-01-01", "2023-12-31") == -1);
    CHECK(daysBetween("2023-12-31", "2024-01-01") == 1);   // 跨年
    CHECK(daysBetween("2024-02-28", "2024-03-01") == 2);   // 2024 闰年
    endCase();
}

static void test_add_days()
{
    beginCase("date: addDays 基本算术");
    CHECK(addDays("2024-01-01", 10) == "2024-01-11");
    CHECK(addDays("2024-02-28", 2) == "2024-03-01");       // 闰年
    CHECK(addDays("2023-02-28", 1) == "2023-03-01");       // 平年
    CHECK(addDays("2023-12-31", 1) == "2024-01-01");       // 跨年
    endCase();
}

int main()
{
    std::cout << "===== Fine / Date Tests =====\n";

    test_days_between();
    test_add_days();

    std::cout << "=============================\n";
    std::cout << "PASSED: " << g_passed << "   FAILED: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
```

- [ ] **Step 2: 跑测试确认编译失败**

Run: `make build/test_fine`
Expected: 编译失败，找不到 `DateUtil.h` / `daysBetween` / `addDays`。

- [ ] **Step 3: 写 `include/DateUtil.h`**

```cpp
#pragma once
#include <string>

namespace library
{
    // "YYYY-MM-DD" 之间差多少天，to 晚就是正数
    int daysBetween(const std::string &from, const std::string &to);

    // 把日期往后推 n 天
    std::string addDays(const std::string &date, int n);
}
```

- [ ] **Step 4: 写 `src/DateUtil.cpp`**

```cpp
#include "../include/DateUtil.h"
#include <cstdio>

namespace library
{
    namespace
    {
        // Howard Hinnant 的算法：(y,m,d) 换成自纪元的天数序号
        long long daysFromCivil(int y, unsigned m, unsigned d)
        {
            y -= m <= 2;
            const int era = (y >= 0 ? y : y - 399) / 400;
            const unsigned yoe = static_cast<unsigned>(y - era * 400);
            const unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
            const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
            return era * 146097LL + static_cast<long long>(doe) - 719468;
        }

        // 反过来：天数序号还原成 (y,m,d)
        void civilFromDays(long long z, int &y, unsigned &m, unsigned &d)
        {
            z += 719468;
            const long long era = (z >= 0 ? z : z - 146096) / 146097;
            const unsigned doe = static_cast<unsigned>(z - era * 146097);
            const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
            y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
            const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
            const unsigned mp = (5 * doy + 2) / 153;
            d = doy - (153 * mp + 2) / 5 + 1;
            m = mp + (mp < 10 ? 3 : -9);
            y += (m <= 2);
        }

        long long serial(const std::string &s)
        {
            int y = 0, mo = 0, da = 0;
            std::sscanf(s.c_str(), "%d-%d-%d", &y, &mo, &da);
            return daysFromCivil(y, static_cast<unsigned>(mo), static_cast<unsigned>(da));
        }
    }

    int daysBetween(const std::string &from, const std::string &to)
    {
        return static_cast<int>(serial(to) - serial(from));
    }

    std::string addDays(const std::string &date, int n)
    {
        int y = 0;
        unsigned m = 0, d = 0;
        civilFromDays(serial(date) + n, y, m, d);
        char buf[11];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y,
                      static_cast<int>(m), static_cast<int>(d));
        return std::string(buf);
    }
}
```

- [ ] **Step 5: 跑测试确认通过**

Run: `make build/test_fine && ./build/test_fine`
Expected: PASS，`FAILED: 0`。

- [ ] **Step 6: 提交**

```bash
git add include/DateUtil.h src/DateUtil.cpp test/test_fine.cpp
git commit -m "feat：新增 DateUtil 日期天数算术"
```

---

### Task 2: 图书借期多态 `Book::maxBorrowDays()`

**Files:**
- Modify: `include/book/Book.h`（加纯虚 `maxBorrowDays`）
- Modify: `include/book/PhysicalBook.h` / `include/book/EBook.h` / `include/book/Magazine.h`（各加 override 声明）
- Modify: `src/book/PhysicalBook.cpp` / `src/book/EBook.cpp` / `src/book/Magazine.cpp`（实现 + 顺手更新 `availabilityInfo`）
- Test: `test/test_fine.cpp`（追加图书借期用例）

**Interfaces:**
- Consumes: 无
- Produces: `int Book::maxBorrowDays() const`（多态）。`PhysicalBook` 返回 `30`，`EBook` 返回 `-1`，`Magazine` 返回 `-1`。

- [ ] **Step 1: 在 `test/test_fine.cpp` 加失败测试**

在 `#include` 区追加：
```cpp
#include "../include/book/PhysicalBook.h"
#include "../include/book/EBook.h"
#include "../include/book/Magazine.h"
#include <memory>
```

在 `test_add_days()` 之后加：
```cpp
static void test_book_borrow_days()
{
    beginCase("book: maxBorrowDays 多态");
    PhysicalBook p("P1", "C++ Primer", "Lippman", "AW", 2);
    EBook e("E1", "SICP", "Abelson", "MIT", "PDF", 12.5);
    Magazine m("M1", "Nature", "NPG", 42, "2024-01-01");

    CHECK(p.maxBorrowDays() == 30);
    CHECK(e.maxBorrowDays() < 0);   // 电子书不计逾期
    CHECK(m.maxBorrowDays() < 0);   // 杂志不外借
    endCase();
}
```

在 `main()` 里 `test_add_days();` 之后加：
```cpp
    test_book_borrow_days();
```

- [ ] **Step 2: 跑测试确认编译失败**

Run: `make build/test_fine`
Expected: 编译失败，`Book` 无 `maxBorrowDays` 成员。

- [ ] **Step 3: `include/book/Book.h` 加纯虚接口**

在 `[[nodiscard]] virtual std::string availabilityInfo() const = 0;` 之后加一行：
```cpp
        // 借出最大期限（天），<0 表示不计逾期/不外借
        [[nodiscard]] virtual int maxBorrowDays() const = 0;
```

- [ ] **Step 4: 三个子类头文件各加 override 声明**

`include/book/PhysicalBook.h`、`include/book/EBook.h`、`include/book/Magazine.h` 各自在 `availabilityInfo` 声明附近加：
```cpp
        [[nodiscard]] int maxBorrowDays() const override;
```

- [ ] **Step 5: 三个子类 `.cpp` 实现并更新 availabilityInfo**

`src/book/PhysicalBook.cpp`：把 `availabilityInfo` 改成带借期，并加 `maxBorrowDays`：
```cpp
    std::string PhysicalBook::availabilityInfo() const
    {
        return std::to_string(availableCopies_) + "/" +
               std::to_string(totalCopies_) + " 可借 · 借期30天";
    }

    int PhysicalBook::maxBorrowDays() const
    {
        return 30;
    }
```

`src/book/EBook.cpp`：
```cpp
    std::string EBook::availabilityInfo() const
    {
        return "电子资源（无限借阅，不计逾期）";
    }

    int EBook::maxBorrowDays() const
    {
        return -1;
    }
```

`src/book/Magazine.cpp`：
```cpp
    int Magazine::maxBorrowDays() const
    {
        return -1;
    }
```
（`Magazine::availabilityInfo` 保持 `"仅限馆内阅览"` 不变。）

- [ ] **Step 6: 跑全部测试确认通过**

Run: `make test`
Expected: 所有测试（含 `test_fine`、原有 `test_book/test_library/...`）PASS。

- [ ] **Step 7: 提交**

```bash
git add include/book src/book test/test_fine.cpp
git commit -m "feat：图书新增 maxBorrowDays 多态借期"
```

---

### Task 3: 读者借期与日罚多态

**Files:**
- Modify: `include/reader/Reader.h`（加两个纯虚接口）
- Modify: `include/reader/Student.h` / `include/reader/Teacher.h`（加 override 声明）
- Modify: `src/reader/Student.cpp` / `src/reader/Teacher.cpp`（实现）
- Modify: `src/reader/Reader.cpp`（`display` 顺手展示借期与日罚）
- Test: `test/test_fine.cpp`（追加读者用例）

**Interfaces:**
- Consumes: 无
- Produces:
  - `int Reader::maxBorrowDays() const`（多态）：`StudentReader` 返回 `30`，`TeacherReader` 返回 `60`。
  - `double Reader::finePerDay() const`（多态）：`StudentReader` 返回 `0.50`，`TeacherReader` 返回 `0.20`。

- [ ] **Step 1: 在 `test/test_fine.cpp` 加失败测试**

`#include` 区追加：
```cpp
#include "../include/reader/Student.h"
#include "../include/reader/Teacher.h"
```

在 `test_book_borrow_days()` 之后加：
```cpp
static void test_reader_terms()
{
    beginCase("reader: 借期与日罚多态");
    StudentReader s("S1", "Alice");
    TeacherReader t("T1", "Bob");

    CHECK(s.maxBorrowDays() == 30);
    CHECK(t.maxBorrowDays() == 60);
    CHECK(approx(s.finePerDay(), 0.50));
    CHECK(approx(t.finePerDay(), 0.20));
    endCase();
}
```

`main()` 里 `test_book_borrow_days();` 之后加：
```cpp
    test_reader_terms();
```

- [ ] **Step 2: 跑测试确认编译失败**

Run: `make build/test_fine`
Expected: 编译失败，`Reader` 无 `maxBorrowDays` / `finePerDay`。

- [ ] **Step 3: `include/reader/Reader.h` 加纯虚接口**

在 `virtual int maxBorrowLimit() const = 0;` 那行之后加：
```cpp
        // 最长借书期限（天）
        [[nodiscard]] virtual int maxBorrowDays() const = 0;
        // 每天超期罚款（元）
        [[nodiscard]] virtual double finePerDay() const = 0;
```

- [ ] **Step 4: 子类头文件加 override 声明**

`include/reader/Student.h` 与 `include/reader/Teacher.h` 各在 `maxBorrowLimit` 声明附近加：
```cpp
        [[nodiscard]] int maxBorrowDays() const override;
        [[nodiscard]] double finePerDay() const override;
```

- [ ] **Step 5: 子类 `.cpp` 实现**

`src/reader/Student.cpp` 加：
```cpp
    int StudentReader::maxBorrowDays() const
    {
        return 30;
    }

    double StudentReader::finePerDay() const
    {
        return 0.50;
    }
```

`src/reader/Teacher.cpp` 加：
```cpp
    int TeacherReader::maxBorrowDays() const
    {
        return 60;
    }

    double TeacherReader::finePerDay() const
    {
        return 0.20;
    }
```

- [ ] **Step 6: `src/reader/Reader.cpp` 的 `display` 加一行借期/日罚**

把 `display` 改成（在原输出后追加借期与日罚）：
```cpp
    void Reader::display() const
    {
        std::cout << "[" << getType() << "] "
                  << getName()
                  << " 已借 " << borrowedCount()
                  << "/" << maxBorrowLimit() << " 本"
                  << " · 借期" << maxBorrowDays() << "天"
                  << " · 逾期" << finePerDay() << "元/天"
                  << std::endl;
    }
```

- [ ] **Step 7: 跑全部测试确认通过**

Run: `make test`
Expected: 全部 PASS。

- [ ] **Step 8: 提交**

```bash
git add include/reader src/reader test/test_fine.cpp
git commit -m "feat：读者新增 maxBorrowDays 与 finePerDay 多态"
```

---

### Task 4: `LibrarySystem::calculateFine` 逾期费用计算

**Files:**
- Modify: `include/LibrarySystem.h`（`today()` 改 public、加 `calculateFine` 声明）
- Modify: `src/LibrarySystem.cpp`（实现 `calculateFine`，加 include）
- Test: `test/test_fine.cpp`（追加 calculateFine 用例 + 本地建系统辅助）

**Interfaces:**
- Consumes: `daysBetween`、`addDays`、`Book::maxBorrowDays`、`Reader::maxBorrowDays`、`Reader::finePerDay`。
- Produces: `double LibrarySystem::calculateFine(const std::string& readerId, const std::string& bookId, const std::string& asOfDate) const` —— 找不到读者/图书抛对应异常；无未归还记录或图书 `maxBorrowDays()<0` 或未逾期，均返回 `0.0`；否则 `逾期天数 × finePerDay()`。`static std::string LibrarySystem::today()` 变为 public。

- [ ] **Step 1: 在 `test/test_fine.cpp` 加失败测试**

`#include` 区追加：
```cpp
#include "../include/LibrarySystem.h"
#include "../include/LibraryException.h"
```

在 `test_reader_terms()` 之后加本地建系统辅助与用例：
```cpp
namespace
{
    // P1 纸质书(借期30)，E1 电子书；S1 学生(借期30/0.5)，T1 教师(借期60/0.2)
    std::unique_ptr<LibrarySystem> makeFineSystem()
    {
        auto sys = std::make_unique<LibrarySystem>();
        sys->addBook(std::make_unique<PhysicalBook>("P1", "C++ Primer", "Lippman", "AW", 2));
        sys->addBook(std::make_unique<EBook>("E1", "SICP", "Abelson", "MIT", "PDF", 12.5));
        sys->addReader(std::make_unique<StudentReader>("S1", "Alice"));
        sys->addReader(std::make_unique<TeacherReader>("T1", "Bob"));
        return sys;
    }
}

static void test_fine_student_overdue()
{
    beginCase("fine: 学生纸质书逾期 10 天 = 5.0 元");
    auto sys = makeFineSystem();
    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::Success);
    std::string borrowDate = sys->getRecords().back().borrowDate;
    // 借期 min(P1=30, S1=30)=30，借后 40 天即逾期 10 天
    std::string asOf = addDays(borrowDate, 40);
    CHECK(approx(sys->calculateFine("S1", "P1", asOf), 5.0));
    endCase();
}

static void test_fine_teacher_uses_min_term()
{
    beginCase("fine: 教师借纸质书有效借期取 min(30,60)=30");
    auto sys = makeFineSystem();
    CHECK(sys->borrowBook("T1", "P1") == BorrowStatus::Success);
    std::string borrowDate = sys->getRecords().back().borrowDate;
    // 若取 60 天则未逾期；取 30 天则逾期 10 天 × 0.2 = 2.0
    std::string asOf = addDays(borrowDate, 40);
    CHECK(approx(sys->calculateFine("T1", "P1", asOf), 2.0));
    endCase();
}

static void test_fine_ebook_never()
{
    beginCase("fine: 电子书永不逾期");
    auto sys = makeFineSystem();
    CHECK(sys->borrowBook("S1", "E1") == BorrowStatus::Success);
    std::string borrowDate = sys->getRecords().back().borrowDate;
    std::string asOf = addDays(borrowDate, 365);
    CHECK(approx(sys->calculateFine("S1", "E1", asOf), 0.0));
    endCase();
}

static void test_fine_not_overdue()
{
    beginCase("fine: 借期内不收费");
    auto sys = makeFineSystem();
    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::Success);
    std::string borrowDate = sys->getRecords().back().borrowDate;
    std::string asOf = addDays(borrowDate, 20);   // 借期 30 天内
    CHECK(approx(sys->calculateFine("S1", "P1", asOf), 0.0));
    endCase();
}

static void test_fine_no_record()
{
    beginCase("fine: 没借过这本书返回 0");
    auto sys = makeFineSystem();
    CHECK(approx(sys->calculateFine("S1", "P1", "2099-01-01"), 0.0));
    endCase();
}

static void test_fine_throws()
{
    beginCase("fine: 找不到读者/图书抛异常");
    auto sys = makeFineSystem();
    bool caught = false;
    try { sys->calculateFine("nope", "P1", "2099-01-01"); }
    catch (const ReaderNotFoundException &) { caught = true; }
    catch (...) {}
    CHECK(caught);

    caught = false;
    try { sys->calculateFine("S1", "nope", "2099-01-01"); }
    catch (const BookNotFoundException &) { caught = true; }
    catch (...) {}
    CHECK(caught);
    endCase();
}
```

`main()` 里 `test_reader_terms();` 之后加：
```cpp
    test_fine_student_overdue();
    test_fine_teacher_uses_min_term();
    test_fine_ebook_never();
    test_fine_not_overdue();
    test_fine_no_record();
    test_fine_throws();
```

- [ ] **Step 2: 跑测试确认编译失败**

Run: `make build/test_fine`
Expected: 编译失败，`LibrarySystem` 无 `calculateFine`。

- [ ] **Step 3: `include/LibrarySystem.h` 调整**

把 `private:` 区里的 `static std::string today();` 删除，挪到 `public:` 区（放在 `~LibrarySystem()` 之后即可）：
```cpp
        // 系统当天日期 "YYYY-MM-DD"，菜单与还书流程用它当 asOf
        static std::string today();
```

在 `// ---- 查询接口 ----` 区（`getRecords` 附近）加声明：
```cpp
        // 某读者所借某书截至 asOfDate 的逾期费用（元）
        [[nodiscard]] double calculateFine(const std::string &readerId,
                                           const std::string &bookId,
                                           const std::string &asOfDate) const;
```

- [ ] **Step 4: `src/LibrarySystem.cpp` 实现**

文件顶部 include 区加：
```cpp
#include "../include/DateUtil.h"
#include <algorithm>
```

在 `returnBook` 实现之后加：
```cpp
    double LibrarySystem::calculateFine(const std::string &readerId,
                                        const std::string &bookId,
                                        const std::string &asOfDate) const
    {
        Reader *reader = findReader(readerId);
        if (!reader)
            throw ReaderNotFoundException(readerId);

        Book *book = findBook(bookId);
        if (!book)
            throw BookNotFoundException(bookId);

        // 找这本书还没还的那条记录（从最近往前找）
        const BorrowingRecord *rec = nullptr;
        for (auto it = records_.rbegin(); it != records_.rend(); ++it)
        {
            if (it->readerId == readerId && it->bookId == bookId && !it->isReturned())
            {
                rec = &(*it);
                break;
            }
        }
        if (!rec)
            return 0.0;

        // 电子书/杂志不计逾期
        if (book->maxBorrowDays() < 0)
            return 0.0;

        int dueDays = std::min(book->maxBorrowDays(), reader->maxBorrowDays());
        int overdue = daysBetween(rec->borrowDate, asOfDate) - dueDays;
        if (overdue <= 0)
            return 0.0;

        return overdue * reader->finePerDay();
    }
```

- [ ] **Step 5: 跑全部测试确认通过**

Run: `make test`
Expected: 全部 PASS，`test_fine` 含 6 个 fine 用例通过。

- [ ] **Step 6: 提交**

```bash
git add include/LibrarySystem.h src/LibrarySystem.cpp test/test_fine.cpp
git commit -m "feat：LibrarySystem 新增逾期费用计算 calculateFine"
```

---

### Task 5: 菜单接入与还书展示逾期费用 + README

**Files:**
- Modify: `main.cpp`（新增 `doFine`、菜单项 9、还书后展示逾期费用、金额两位小数）
- Modify: `README.md`（补充逾期费用功能说明）
- 手动验证（菜单为交互逻辑，无单测）

**Interfaces:**
- Consumes: `LibrarySystem::calculateFine`、`LibrarySystem::today`。
- Produces: 无（仅 CLI 行为）。

- [ ] **Step 1: `main.cpp` 顶部加 `<iomanip>`**

在 `#include <string>` 之后加：
```cpp
#include <iomanip>
```

- [ ] **Step 2: 改 `doReturn` —— 还书前先查逾期费用**

把现有 `doReturn` 整体替换为：
```cpp
    void doReturn(LibrarySystem &sys)
    {
        std::cout << "\n-- 还书 --\n";
        std::string readerId = readLine("读者ID: ");
        std::string bookId = readLine("图书ID: ");
        try
        {
            // 还书会把记录标记成已还，所以先算逾期费用再还
            double fine = sys.calculateFine(readerId, bookId, LibrarySystem::today());
            ReturnStatus s = sys.returnBook(readerId, bookId);
            reportReturn(s);
            if (s == ReturnStatus::Success && fine > 0.0)
                std::cout << "  [!] 逾期费用: " << std::fixed << std::setprecision(2)
                          << fine << " 元\n";
        }
        catch (const LibraryException &e)
        {
            std::cout << "  [X] " << e.what() << '\n';
        }
    }
```

- [ ] **Step 3: 加 `doFine` 动作（放在 `doListRecords` 之前）**

```cpp
    void doFine(LibrarySystem &sys)
    {
        std::cout << "\n-- 逾期费用查询 --\n";
        std::string readerId = readLine("读者ID: ");
        std::string bookId = readLine("图书ID: ");
        try
        {
            double fine = sys.calculateFine(readerId, bookId, LibrarySystem::today());
            if (fine > 0.0)
                std::cout << "  逾期费用: " << std::fixed << std::setprecision(2)
                          << fine << " 元\n";
            else
                std::cout << "  未逾期，无需缴费\n";
        }
        catch (const LibraryException &e)
        {
            std::cout << "  [X] " << e.what() << '\n';
        }
    }
```

- [ ] **Step 4: 菜单加第 9 项**

`printMenu` 里在 `"  8) 保存数据\n"` 之后、`"  0) 保存并退出\n"` 之前插入：
```cpp
                  << "  9) 逾期费用查询\n"
```

`main()` 的 `switch` 里在 `case 8:` 块之后、`case 0:` 之前插入：
```cpp
        case 9:
            doFine(sys);
            break;
```

- [ ] **Step 5: 编译并手动验证**

Run:
```bash
make
printf '2\n1\nS9\n张三\n1\n1\nP9\n测试书\n作者\n社\n1\n3\nS9\nP9\n9\nS9\nP9\n0\n' | ./library
```
Expected: 能添加学生 S9、纸质书 P9、借阅成功；菜单 9 查询 P9 因当天借当天未逾期，显示"未逾期，无需缴费"；最后保存退出无报错。

（说明：当天借阅无法立即制造逾期，手动验证只确认流程跑通；逾期金额的正确性已由 Task 4 单测覆盖。）

- [ ] **Step 6: 还原手动验证写入的数据**

Run:
```bash
git checkout -- data/
```
Expected: `data/*.csv` 回到提交前状态（手动验证不污染仓库数据）。

- [ ] **Step 7: 更新 `README.md`**

在功能列表/菜单说明处补充一句逾期相关说明（向 README 现有语气看齐），例如在功能点列表加：
```markdown
- 逾期费用计算：按 `min(图书借期, 读者借期)` 判定到期日，超期按读者类型的日罚标准计费（学生 0.50 元/天、教师 0.20 元/天；电子书/杂志不计逾期）。菜单「9) 逾期费用查询」可随时查询，还书时自动提示。
```

- [ ] **Step 8: 跑全部测试确认仍通过**

Run: `make test`
Expected: 全部 PASS。

- [ ] **Step 9: 提交**

```bash
git add main.cpp README.md
git commit -m "feat：菜单接入逾期费用查询，还书提示逾期费用"
```

---

## 验收（全部完成后）

- `make test` 全绿，含新增 `test_fine` 且原有测试未改动。
- `make` 在 `-Wall -Wextra` 下无警告。
- 菜单「9) 逾期费用查询」与还书流程能展示逾期费用。
- 学生/教师在借期、日罚上表现差异；电子书永不逾期；借期取图书与读者的较小值。
