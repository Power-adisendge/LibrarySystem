// test/test_storage.cpp
//
// CSV 持久化往返测试：建系统 -> 借阅产生动态状态 -> save -> 新系统 load
// -> 断言动态状态（库存/借阅列表/下载数/记录）被完整还原。
// 复用 test_library.cpp 同款轻量断言框架。

#include "../include/LibrarySystem.h"
#include "../include/book/EBook.h"
#include "../include/book/Magazine.h"
#include "../include/book/PhysicalBook.h"
#include "../include/reader/Student.h"
#include "../include/reader/Teacher.h"

#include <cstdio>
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
}

#define CHECK(cond)                                \
    do                                             \
    {                                              \
        if (cond) { ++g_passed; }                  \
        else { reportFail(#cond, __FILE__, __LINE__); } \
    } while (0)

namespace
{
    // 临时目录前缀（测试结束清理）
    const std::string kDir = "build/test_data";

    std::unique_ptr<LibrarySystem> makeSystem()
    {
        auto sys = std::make_unique<LibrarySystem>();
        sys->addBook(std::make_unique<PhysicalBook>("P1", "C++ Primer", "Lippman", "Addison-Wesley", 2));
        sys->addBook(std::make_unique<PhysicalBook>("P2", "Effective C++", "Meyers", "Addison-Wesley", 1));
        sys->addBook(std::make_unique<EBook>("E1", "SICP", "Abelson", "MIT", "PDF", 12.5));
        sys->addBook(std::make_unique<Magazine>("M1", "Nature", "NPG", 42, "2024-01-01"));
        sys->addReader(std::make_unique<StudentReader>("S1", "Alice"));
        sys->addReader(std::make_unique<TeacherReader>("T1", "Bob"));
        return sys;
    }
}

// 往返：建系统 -> 借阅 -> save -> load 到新系统 -> 比对
static void test_roundtrip_restores_state()
{
    beginCase("storage: save/load 往返还原动态状态");

    // ---- 1. 原始系统，制造动态状态 ----
    auto sys = makeSystem();
    CHECK(sys->borrowBook("S1", "P1") == BorrowStatus::Success); // P1 库存 2->1
    CHECK(sys->borrowBook("S1", "P2") == BorrowStatus::Success); // P2 库存 1->0
    CHECK(sys->borrowBook("S1", "E1") == BorrowStatus::Success); // E1 下载 +1
    CHECK(sys->borrowBook("T1", "E1") == BorrowStatus::Success); // E1 下载 +1 (共2)
    CHECK(sys->returnBook("S1", "P1") == ReturnStatus::Success); // P1 还回 ->2, S1 借阅列表移除 P1

    // 此刻：P1=2/2, P2=0/1, E1 下载=2; S1 借了 {P2,E1}, T1 借了 {E1}
    sys->save(kDir);

    // ---- 2. 新系统加载（空系统，load 负责重建一切）----
    auto fresh = std::make_unique<LibrarySystem>();
    fresh->load(kDir);

    // ---- 3. 静态结构还原 ----
    CHECK(fresh->bookCount() == 4);
    CHECK(fresh->readerCount() == 2);

    // ---- 4. 动态状态还原 ----
    auto *p1 = dynamic_cast<PhysicalBook *>(fresh->findBook("P1"));
    auto *p2 = dynamic_cast<PhysicalBook *>(fresh->findBook("P2"));
    auto *e1 = dynamic_cast<EBook *>(fresh->findBook("E1"));
    CHECK(p1 != nullptr && p1->getAvailableCopies() == 2); // 借后又还 -> 满
    CHECK(p2 != nullptr && p2->getAvailableCopies() == 0); // 仍借出
    CHECK(e1 != nullptr && e1->getDownloadCount() == 2);   // 历史借 2 次

    auto *s1 = fresh->findReader("S1");
    auto *t1 = fresh->findReader("T1");
    CHECK(s1 != nullptr && s1->getType() == "StudentReader");
    CHECK(t1 != nullptr && t1->getType() == "TeacherReader");
    CHECK(s1->borrowedCount() == 2 && s1->hasBorrowed("P2") && s1->hasBorrowed("E1"));
    CHECK(!s1->hasBorrowed("P1")); // 已归还，不在列表
    CHECK(t1->borrowedCount() == 1 && t1->hasBorrowed("E1"));

    // ---- 5. 记录还原（4 借 + P1 那条标记已还）----
    CHECK(fresh->getRecords().size() == 4);
    int returnedCount = 0;
    for (const auto &r : fresh->getRecords())
        if (r.isReturned()) ++returnedCount;
    CHECK(returnedCount == 1);

    // ---- 6. 还原后业务仍正确：P2 已借满，再借应 unavailable ----
    CHECK(fresh->borrowBook("T1", "P2") == BorrowStatus::BookUnavailable);

    endCase();
}

// 空系统加载不存在的目录：应安全（无文件视为无数据），不崩溃
static void test_load_missing_is_safe()
{
    beginCase("storage: 加载不存在的数据目录安全无崩溃");
    auto sys = std::make_unique<LibrarySystem>();
    sys->load("build/no_such_dir_xyz");
    CHECK(sys->bookCount() == 0);
    CHECK(sys->readerCount() == 0);
    CHECK(sys->getRecords().empty());
    endCase();
}

int main()
{
    std::cout << "===== Storage Tests =====\n";
    test_roundtrip_restores_state();
    test_load_missing_is_safe();
    std::cout << "=========================\n";
    std::cout << "PASSED: " << g_passed << "   FAILED: " << g_failed << "\n";

    // 清理测试产生的 CSV
    std::remove((kDir + "/books.csv").c_str());
    std::remove((kDir + "/readers.csv").c_str());
    std::remove((kDir + "/records.csv").c_str());

    return g_failed == 0 ? 0 : 1;
}
