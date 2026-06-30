// test/test_fine.cpp
// 逾期费用相关测试，沿用 test_library 的手写断言风格
#include "../include/DateUtil.h"
#include "../include/book/PhysicalBook.h"
#include "../include/book/EBook.h"
#include "../include/book/Magazine.h"
#include "../include/reader/Student.h"
#include "../include/reader/Teacher.h"
#include "../include/LibrarySystem.h"
#include "../include/LibraryException.h"

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
    try { (void)sys->calculateFine("nope", "P1", "2099-01-01"); }
    catch (const ReaderNotFoundException &) { caught = true; }
    catch (...) {}
    CHECK(caught);

    caught = false;
    try { (void)sys->calculateFine("S1", "nope", "2099-01-01"); }
    catch (const BookNotFoundException &) { caught = true; }
    catch (...) {}
    CHECK(caught);
    endCase();
}

int main()
{
    std::cout << "===== Fine / Date Tests =====\n";

    test_days_between();
    test_add_days();
    test_book_borrow_days();
    test_reader_terms();
    test_fine_student_overdue();
    test_fine_teacher_uses_min_term();
    test_fine_ebook_never();
    test_fine_not_overdue();
    test_fine_no_record();
    test_fine_throws();

    std::cout << "=============================\n";
    std::cout << "PASSED: " << g_passed << "   FAILED: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
