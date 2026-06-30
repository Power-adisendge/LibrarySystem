#pragma once

#include "book/Book.h"
#include "reader/Reader.h"
#include "BorrowingRecord.h"
#include "LibraryException.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace library
{

    // 借书的结果（找不到实体的情况已经由异常处理了）
    enum class BorrowStatus
    {
        Success,         // 借阅成功
        BookUnavailable, // 书存在但当前无库存
        LimitReached,    // 读者已达借阅上限
        AlreadyBorrowed  // 该读者已借过这本书
    };

    // 还书的结果
    enum class ReturnStatus
    {
        Success,    // 还书成功
        NotBorrowed // 该读者未借过这本书
    };

    /**
     * @brief 图书馆系统的总调度中心
     *
     * 通过 unique_ptr 独占 Book 和 Reader 的所有权，
     * 是系统中唯一负责内存的地方。
     *
     * 对外提供借阅/归还接口，内部协调 Book 和 Reader 的状态变更，
     * 并维护完整的借阅记录。
     */
    class LibrarySystem
    {
    private:
        std::unordered_map<std::string, std::unique_ptr<Book>> books_;
        std::unordered_map<std::string, std::unique_ptr<Reader>> readers_;
        std::vector<BorrowingRecord> records_;

    public:
        LibrarySystem() = default;
        ~LibrarySystem() = default;

        // 系统当天日期 "YYYY-MM-DD"，菜单和还书流程拿它当 asOf
        static std::string today();

        // 系统本身也应该是唯一的，禁用复制，和实体的逻辑一致
        LibrarySystem(const LibrarySystem &) = delete;
        LibrarySystem &operator=(const LibrarySystem &) = delete;

        void addBook(std::unique_ptr<Book> book);
        void addReader(std::unique_ptr<Reader> reader);

        // 找不到返回 nullptr，不抛异常
        [[nodiscard]] Book *findBook(const std::string &id) const;
        [[nodiscard]] Reader *findReader(const std::string &id) const;

        // ---- 核心业务逻辑 ----
        // 找不到书/读者会抛 BookNotFoundException / ReaderNotFoundException（写在LibraryException.h里面）
        BorrowStatus borrowBook(const std::string &readerId, const std::string &bookId);
        ReturnStatus returnBook(const std::string &readerId, const std::string &bookId);

        // ---- 查询接口 ----
        [[nodiscard]] const std::vector<BorrowingRecord> &getRecords() const
        {
            return records_;
        }
        [[nodiscard]] std::size_t bookCount() const { return books_.size(); }
        [[nodiscard]] std::size_t readerCount() const { return readers_.size(); }

        // 某读者所借某书截至 asOfDate 的逾期费用（元）
        [[nodiscard]] double calculateFine(const std::string &readerId,
                                           const std::string &bookId,
                                           const std::string &asOfDate) const;

        // 列出所有图书/读者（多态）
        void listBooks() const;
        void listReaders() const;

        // ---- 持久化（委托给 CsvStorage）----
        void save(const std::string &dir = "data") const;
        void load(const std::string &dir = "data");
    };
}