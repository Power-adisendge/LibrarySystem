#include "../include/LibrarySystem.h"

#include <chrono>
#include <ctime>
#include <iostream>

namespace library
{
    /**
     * @details 这里是一个时间系统
     *
     * 用于直接获取系统时间，打上借阅的record
     */

    std::string LibrarySystem::today()
    {
        using namespace std::chrono;
        std::time_t t = system_clock::to_time_t(system_clock::now());
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buf[11]; // "YYYY-MM-DD"这个是统一的格式
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
        return std::string(buf);
    }

    // ---- 添加实体 ----
    void LibrarySystem::addBook(std::unique_ptr<Book> book)
    {
        if (!book)
            return;

        const std::string id = book->getId();
        books_[id] = std::move(book);
    }

    void LibrarySystem::addReader(std::unique_ptr<Reader> reader)
    {
        if (!reader)
            return;

        const std::string id = reader->getId();
        readers_[id] = std::move(reader);
    }

    // ---- 查询 ----
    Book *LibrarySystem::findBook(const std::string &id) const
    {
        auto it = books_.find(id);
        return (it == books_.end()) ? nullptr : it->second.get();
    }

    Reader *LibrarySystem::findReader(const std::string &id) const
    {
        auto it = readers_.find(id);
        return (it == readers_.end()) ? nullptr : it->second.get();
    }

    // ---- 借书 ----
    BorrowStatus LibrarySystem::borrowBook(const std::string &readerId, const std::string &bookId)
    {
        // 1. 定位实体，找不到抛异常
        Reader *reader = findReader(readerId);
        if (!reader)
            throw ReaderNotFoundException(readerId);

        Book *book = findBook(bookId);
        if (!book)
            throw BookNotFoundException(bookId);

        // 2. 库存检查（多态：不同图书库存逻辑不同）
        if (!book->isBorrowable())
            return BorrowStatus::BookUnavailable;

        // 3. 交给读者记账
        BorrowResult r = reader->borrow(bookId);
        switch (r)
        {
        case BorrowResult::LimitReached:
            return BorrowStatus::LimitReached;
        case BorrowResult::AlreadyBorrowed:
            return BorrowStatus::AlreadyBorrowed;
        case BorrowResult::Success:
            break;
        case BorrowResult::NotBorrowed:
            return BorrowStatus::AlreadyBorrowed;
        }

        // 4. 扣减库存
        if (!book->borrowOut())
        {
            reader->returnBook(bookId); // 回滚读者侧的记账
            return BorrowStatus::BookUnavailable;
        }

        // 5. 记录
        records_.emplace_back(readerId, bookId, today());
        return BorrowStatus::Success;
    }

    // ---- 还书逻辑 ----
    ReturnStatus LibrarySystem::returnBook(const std::string &readerId,
                                           const std::string &bookId)
    {
        Reader *reader = findReader(readerId);
        if (!reader)
            throw ReaderNotFoundException(readerId);

        Book *book = findBook(bookId);
        if (!book)
            throw BookNotFoundException(bookId);

        BorrowResult r = reader->returnBook(bookId);
        if (r == BorrowResult::NotBorrowed)
            return ReturnStatus::NotBorrowed;

        book->returnBack();

        for (auto it = records_.rbegin(); it != records_.rend(); ++it)
        {
            if (it->readerId == readerId && it->bookId == bookId && !it->isReturned())
            {
                it->returnDate = today();
                break;
            }
        }
        return ReturnStatus::Success;
    }

    void LibrarySystem::listBooks() const
    {
        std::cout << "===== Books (" << books_.size() << ") =====\n";
        for (const auto &[id, book] : books_)
            book->display();
    }

    void LibrarySystem::listReaders() const
    {
        std::cout << "===== Readers (" << readers_.size() << ") =====\n";
        for (const auto &[id, reader] : readers_)
            reader->display();
    }

}