#include "include/LibrarySystem.h"
#include "include/LibraryException.h"
#include "include/book/PhysicalBook.h"
#include "include/book/EBook.h"
#include "include/book/Magazine.h"
#include "include/reader/Student.h"
#include "include/reader/Teacher.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>

namespace
{
    using namespace library;

    constexpr const char *kDataDir = "data";

    // ---- 输入辅助：稳健读取，过滤掉残留换行 ----
    std::string readLine(const std::string &prompt)
    {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    int readInt(const std::string &prompt)
    {
        while (true)
        {
            std::cout << prompt;
            std::string line;
            if (!std::getline(std::cin, line))
                return -1;
            try
            {
                return std::stoi(line);
            }
            catch (const std::exception &)
            {
                std::cout << "  [!] 请输入有效数字\n";
            }
        }
    }

    double readDouble(const std::string &prompt)
    {
        while (true)
        {
            std::cout << prompt;
            std::string line;
            if (!std::getline(std::cin, line))
                return 0.0;
            try
            {
                return std::stod(line);
            }
            catch (const std::exception &)
            {
                std::cout << "  [!] 请输入有效数字\n";
            }
        }
    }

    // ---- 借书结果的可读化 ----
    void reportBorrow(BorrowStatus s)
    {
        switch (s)
        {
        case BorrowStatus::Success:
            std::cout << "  [OK] 借阅成功\n";
            break;
        case BorrowStatus::BookUnavailable:
            std::cout << "  [X] 该图书当前不可借（无库存或仅限馆内阅览）\n";
            break;
        case BorrowStatus::LimitReached:
            std::cout << "  [X] 已达借阅上限\n";
            break;
        case BorrowStatus::AlreadyBorrowed:
            std::cout << "  [X] 该读者已借过这本书\n";
            break;
        }
    }

    void reportReturn(ReturnStatus s)
    {
        switch (s)
        {
        case ReturnStatus::Success:
            std::cout << "  [OK] 归还成功\n";
            break;
        case ReturnStatus::NotBorrowed:
            std::cout << "  [X] 该读者并未借过这本书\n";
            break;
        }
    }

    // ============================ 菜单动作 ============================

    void doAddBook(LibrarySystem &sys)
    {
        std::cout << "\n-- 添加图书 --\n"
                  << "  1) 纸质书  2) 电子书  3) 杂志\n";
        int type = readInt("选择类型: ");

        std::string id = readLine("ID: ");
        std::string title = readLine("书名: ");

        if (type == 1)
        {
            std::string author = readLine("作者: ");
            std::string pub = readLine("出版社: ");
            int copies = readInt("总册数: ");
            sys.addBook(std::make_unique<PhysicalBook>(id, title, author, pub, copies));
            std::cout << "  [OK] 已添加纸质书\n";
        }
        else if (type == 2)
        {
            std::string author = readLine("作者: ");
            std::string pub = readLine("出版社: ");
            std::string fmt = readLine("格式(如 PDF/EPUB): ");
            double size = readDouble("文件大小(MB): ");
            sys.addBook(std::make_unique<EBook>(id, title, author, pub, fmt, size));
            std::cout << "  [OK] 已添加电子书\n";
        }
        else if (type == 3)
        {
            std::string pub = readLine("出版社: ");
            int issue = readInt("期号: ");
            std::string date = readLine("出版日期(YYYY-MM-DD): ");
            sys.addBook(std::make_unique<Magazine>(id, title, pub, issue, date));
            std::cout << "  [OK] 已添加杂志\n";
        }
        else
        {
            std::cout << "  [!] 未知类型，已取消\n";
        }
    }

    void doAddReader(LibrarySystem &sys)
    {
        std::cout << "\n-- 添加读者 --\n"
                  << "  1) 学生  2) 教师\n";
        int type = readInt("选择类型: ");
        std::string id = readLine("ID: ");
        std::string name = readLine("姓名: ");

        if (type == 1)
        {
            sys.addReader(std::make_unique<StudentReader>(id, name));
            std::cout << "  [OK] 已添加学生读者\n";
        }
        else if (type == 2)
        {
            sys.addReader(std::make_unique<TeacherReader>(id, name));
            std::cout << "  [OK] 已添加教师读者\n";
        }
        else
        {
            std::cout << "  [!] 未知类型，已取消\n";
        }
    }

    void doBorrow(LibrarySystem &sys)
    {
        std::cout << "\n-- 借书 --\n";
        std::string readerId = readLine("读者ID: ");
        std::string bookId = readLine("图书ID: ");
        try
        {
            reportBorrow(sys.borrowBook(readerId, bookId));
        }
        catch (const LibraryException &e)
        {
            std::cout << "  [X] " << e.what() << '\n';
        }
    }

    void doReturn(LibrarySystem &sys)
    {
        std::cout << "\n-- 还书 --\n";
        std::string readerId = readLine("读者ID: ");
        std::string bookId = readLine("图书ID: ");
        try
        {
            reportReturn(sys.returnBook(readerId, bookId));
        }
        catch (const LibraryException &e)
        {
            std::cout << "  [X] " << e.what() << '\n';
        }
    }

    void doListRecords(const LibrarySystem &sys)
    {
        const auto &recs = sys.getRecords();
        std::cout << "\n===== 借阅记录 (" << recs.size() << ") =====\n";
        for (const auto &r : recs)
            std::cout << "  " << r.toDisplay() << '\n';
    }

    void printMenu()
    {
        std::cout << "\n========== 图书馆管理系统 ==========\n"
                  << "  1) 添加图书\n"
                  << "  2) 添加读者\n"
                  << "  3) 借书\n"
                  << "  4) 还书\n"
                  << "  5) 列出所有图书\n"
                  << "  6) 列出所有读者\n"
                  << "  7) 查看借阅记录\n"
                  << "  8) 保存数据\n"
                  << "  0) 保存并退出\n"
                  << "====================================\n";
    }
}

int main()
{
    LibrarySystem sys;

    // 启动即尝试加载历史数据；文件不存在时 CsvStorage 静默返回空，不抛异常
    sys.load(kDataDir);
    std::cout << "已加载: " << sys.bookCount() << " 本图书, "
              << sys.readerCount() << " 位读者\n";

    while (true)
    {
        printMenu();
        int choice = readInt("请选择: ");

        switch (choice)
        {
        case 1:
            doAddBook(sys);
            break;
        case 2:
            doAddReader(sys);
            break;
        case 3:
            doBorrow(sys);
            break;
        case 4:
            doReturn(sys);
            break;
        case 5:
            std::cout << '\n';
            sys.listBooks();
            break;
        case 6:
            std::cout << '\n';
            sys.listReaders();
            break;
        case 7:
            doListRecords(sys);
            break;
        case 8:
            sys.save(kDataDir);
            std::cout << "  [OK] 数据已保存到 " << kDataDir << "/\n";
            break;
        case 0:
            sys.save(kDataDir);
            std::cout << "数据已保存，再见。\n";
            return 0;
        case -1: // EOF (如管道输入结束)
            sys.save(kDataDir);
            return 0;
        default:
            std::cout << "  [!] 无效选项\n";
            break;
        }
    }
}
