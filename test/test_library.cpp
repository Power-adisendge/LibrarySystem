// 图书馆系统的测试太过于复杂，所以这个cpp文件由ai生成，辅助大作业测试

// test/test_library.cpp
//
// 图书馆系统测试：手写轻量断言框架（无第三方依赖）。
// 直接编进可执行文件，make run 即跑；可在 CodeLLDB 中打断点调试。

#include "../include/LibrarySystem.h"
#include "../include/book/EBook.h"
#include "../include/book/Magazine.h"
#include "../include/book/PhysicalBook.h"
#include "../include/reader/Student.h"
#include "../include/reader/Teacher.h"
#include "../include/LibraryException.h"

#include <iostream>
#include <memory>
#include <string>

using namespace library;

// ---------------- 轻量测试框架 ----------------
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

    void endCase()
    {
        std::cout << "[ OK   ] " << g_currentCase << "\n";
    }

    void reportFail(const char *expr, const char *file, int line)
    {
        ++g_failed;
        std::cout << "[ FAIL ] " << g_currentCase << "\n"
                  << "         " << file << ":" << line << "\n"
                  << "         assertion failed: " << expr << "\n";
    }
}

// CHECK：失败不中断本用例，继续跑后面的断言
#define CHECK(cond)                                \
    do                                             \
    {                                              \
        if (cond)                                  \
        {                                          \
            ++g_passed;                            \
        }                                          \
        else                                       \
        {                                          \
            reportFail(#cond, __FILE__, __LINE__); \
        }                                          \
    } while (0)

// 期望抛出指定异常类型
#define CHECK_THROWS_AS(stmt, ExType)                                   \
    do                                                                  \
    {                                                                   \
        bool caught = false;                                            \
        try                                                             \
        {                                                               \
            stmt;                                                       \
        }                                                               \
        catch (const ExType &)                                          \
        {                                                               \
            caught = true;                                              \
        }                                                               \
        catch (...)                                                     \
        {                                                               \
        }                                                               \
        if (caught)                                                     \
        {                                                               \
            ++g_passed;                                                 \
        }                                                               \
        else                                                            \
        {                                                               \
            reportFail("expected throw: " #ExType, __FILE__, __LINE__); \
        }                                                               \
    } while (0)

// ---------------- 测试辅助：搭一个填好数据的系统 ----------------
namespace
{
    // P1: 库存2 的纸质书；P2: 库存1 的纸质书
    // E1: 电子书；M1: 杂志（不外借）
    // S1: 学生(上限5)；T1: 教师(上限20)
    std::unique_ptr<LibrarySystem> makeSystem()
    {
        auto sys = std::make_unique<LibrarySystem>();

        sys->addBook(std::make_unique<PhysicalBook>(
            "P1", "C++ Primer", "Lippman", "Addison-Wesley", 2));
        sys->addBook(std::make_unique<PhysicalBook>(
            "P2", "Effective C++", "Meyers", "Addison-Wesley", 1));
        sys->addBook(std::make_unique<EBook>(
            "E1", "SICP", "Abelson", "MIT", "PDF", 12.5));
        sys->addBook(std::make_unique<Magazine>(
            "M1", "Nature", "NPG", 42, "2024-01-01"));

        sys->addReader(std::make_unique<StudentReader>("S1", "Alice"));
        sys->addReader(std::make_unique<TeacherReader>("T1", "Bob"));

        return sys;
    }
}

// ---------------- 用例 ----------------

static void test_setup_counts()
{
    beginCase("setup: 数量与查找");
    auto sys = makeSystem();

    CHECK(sys->bookCount() == 4);
    CHECK(sys->readerCount() == 2);

    CHECK(sys->findBook("P1") != nullptr);
    CHECK(sys->findReader("S1") != nullptr);
    CHECK(sys->findBook("nope") == nullptr); // 找不到返回 nullptr，不抛
    CHECK(sys->findReader("nope") == nullptr);

    endCase();
}

static void test_reader_limits()
{
    beginCase("reader: 借阅上限 5 / 20");
    auto sys = makeSystem();

    CHECK(sys->findReader("S1")->maxBorrowLimit() == 5);
    CHECK(sys->findReader("T1")->maxBorrowLimit() == 20);

    CHECK(sys->findReader("S1")->getType() == "StudentReader");
    CHECK(sys->findReader("T1")->getType() == "TeacherReader");

    endCase();
}

static void test_physical_borrow_decrements()
{
    beginCase("physical: 借出使库存减少，归还使库存恢复");
    auto sys = makeSystem();

    auto *p1 = dynamic_cast<PhysicalBook *>(sys->findBook("P1"));
    CHECK(p1 != nullptr);
    CHECK(p1->getAvailableCopies() == 2);

    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::Success);
    CHECK(p1->getAvailableCopies() == 1);
    CHECK(sys->findReader("S1")->hasBorrowed("P1"));
    CHECK(sys->findReader("S1")->borrowedCount() == 1);

    CHECK(sys->returnBook("S1", "P1") == ReturnStatus::Success);
    CHECK(p1->getAvailableCopies() == 2);
    CHECK(!sys->findReader("S1")->hasBorrowed("P1"));
    CHECK(sys->findReader("S1")->borrowedCount() == 0);

    endCase();
}

static void test_physical_out_of_stock()
{
    beginCase("physical: 库存耗尽返回 BookUnavailable");
    auto sys = makeSystem();

    // P2 只有 1 本：S1 借走后，T1 再借应失败
    CHECK(sys->borrowBook("S1", "P2") == BorrowStatus::Success);

    auto *p2 = dynamic_cast<PhysicalBook *>(sys->findBook("P2"));
    CHECK(p2 != nullptr && p2->getAvailableCopies() == 0);
    CHECK(!p2->isBorrowable());

    CHECK(sys->borrowBook("T1", "P2") == BorrowStatus::BookUnavailable);
    CHECK(!sys->findReader("T1")->hasBorrowed("P2"));

    endCase();
}

static void test_already_borrowed()
{
    beginCase("borrow: 同一读者重复借同一本返回 AlreadyBorrowed");
    auto sys = makeSystem();

    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::Success);
    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::AlreadyBorrowed);

    // 重复借不应再次扣库存
    auto *p1 = dynamic_cast<PhysicalBook *>(sys->findBook("P1"));
    CHECK(p1->getAvailableCopies() == 1);

    endCase();
}

static void test_return_not_borrowed()
{
    beginCase("return: 未借过此书返回 NotBorrowed");
    auto sys = makeSystem();

    CHECK(sys->returnBook("S1", "P1") == ReturnStatus::NotBorrowed);

    endCase();
}

static void test_ebook_unlimited()
{
    beginCase("ebook: 可无限借阅，始终 borrowable");
    auto sys = makeSystem();

    auto *e1 = dynamic_cast<EBook *>(sys->findBook("E1"));
    CHECK(e1 != nullptr);
    CHECK(e1->isBorrowable());

    // 两个读者都能借同一本电子书
    CHECK(sys->borrowBook("S1", "E1") == BorrowStatus::Success);
    CHECK(sys->borrowBook("T1", "E1") == BorrowStatus::Success);
    CHECK(e1->isBorrowable()); // 借出后仍可借

    endCase();
}

static void test_magazine_not_borrowable()
{
    beginCase("magazine: 不外借，借阅返回 BookUnavailable");
    auto sys = makeSystem();

    auto *m1 = dynamic_cast<Magazine *>(sys->findBook("M1"));
    CHECK(m1 != nullptr);
    CHECK(!m1->isBorrowable());

    CHECK(sys->borrowBook("S1", "M1") == BorrowStatus::BookUnavailable);
    CHECK(!sys->findReader("S1")->hasBorrowed("M1"));

    endCase();
}

static void test_student_limit_reached()
{
    beginCase("limit: 学生借满 5 本后返回 LimitReached");
    auto sys = makeSystem();

    // 电子书可无限借且不同 id，用来快速塞满借阅额度
    for (int i = 0; i < 5; ++i)
    {
        std::string id = "EX" + std::to_string(i);
        sys->addBook(std::make_unique<EBook>(
            id, "Extra" + std::to_string(i), "Auth", "Pub", "PDF", 1.0));
        CHECK(sys->borrowBook("S1", id) == BorrowStatus::Success);
    }

    CHECK(sys->findReader("S1")->borrowedCount() == 5);

    // 第 6 本应被上限拦下
    sys->addBook(std::make_unique<EBook>(
        "EX5", "Extra5", "Auth", "Pub", "PDF", 1.0));
    CHECK(sys->borrowBook("S1", "EX5") == BorrowStatus::LimitReached);
    CHECK(sys->findReader("S1")->borrowedCount() == 5);

    endCase();
}

static void test_exceptions()
{
    beginCase("exception: 找不到书/读者抛对应异常");
    auto sys = makeSystem();

    CHECK_THROWS_AS(sys->borrowBook("S1", "NoSuchBook"), BookNotFoundException);
    CHECK_THROWS_AS(sys->borrowBook("NoSuchReader", "P1"), ReaderNotFoundException);
    CHECK_THROWS_AS(sys->returnBook("S1", "NoSuchBook"), BookNotFoundException);
    CHECK_THROWS_AS(sys->returnBook("NoSuchReader", "P1"), ReaderNotFoundException);

    // 也应能被基类一网打尽
    CHECK_THROWS_AS(sys->borrowBook("S1", "NoSuchBook"), LibraryException);

    endCase();
}

static void test_records()
{
    beginCase("records: 借阅记录新增与归还标记");
    auto sys = makeSystem();

    CHECK(sys->getRecords().empty());

    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::Success);
    CHECK(sys->getRecords().size() == 1);

    const auto &rec = sys->getRecords().front();
    CHECK(rec.readerId == "S1");
    CHECK(rec.bookId == "P1");
    CHECK(!rec.borrowDate.empty());
    CHECK(!rec.isReturned()); // 尚未归还

    CHECK(sys->returnBook("S1", "P1") == ReturnStatus::Success);
    // 归还后应有一条记录被标记为已还
    bool anyReturned = false;
    for (const auto &r : sys->getRecords())
    {
        if (r.readerId == "S1" && r.bookId == "P1" && r.isReturned())
        {
            anyReturned = true;
        }
    }
    CHECK(anyReturned);

    endCase();
}

// ---------------- main ----------------
int main()
{
    std::cout << "===== Library System Tests =====\n";

    test_setup_counts();
    test_reader_limits();
    test_physical_borrow_decrements();
    test_physical_out_of_stock();
    test_already_borrowed();
    test_return_not_borrowed();
    test_ebook_unlimited();
    test_magazine_not_borrowable();
    test_student_limit_reached();
    test_exceptions();
    test_records();

    std::cout << "================================\n";
    std::cout << "PASSED: " << g_passed << "   FAILED: " << g_failed << "\n";

    return g_failed == 0 ? 0 : 1;
}