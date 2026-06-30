// test/test_fine.cpp
// 逾期费用相关测试，沿用 test_library 的手写断言风格
#include "../include/DateUtil.h"
#include "../include/book/PhysicalBook.h"
#include "../include/book/EBook.h"
#include "../include/book/Magazine.h"

#include <cmath>
#include <iostream>
#include <memory>
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
    [[maybe_unused]] bool approx(double a, double b) { return std::fabs(a - b) < 1e-9; }
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

int main()
{
    std::cout << "===== Fine / Date Tests =====\n";

    test_days_between();
    test_add_days();
    test_book_borrow_days();

    std::cout << "=============================\n";
    std::cout << "PASSED: " << g_passed << "   FAILED: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
