#pragma once
#include <string>

namespace library
{

    /**
     * @brief 一条借阅记录
     *
     * 记录"谁、借了什么、何时借、何时还"
     * returnDate 为空字符串表示尚未归还
     */
    struct BorrowingRecord
    {
        std::string readerId;
        std::string bookId;
        std::string borrowDate;
        std::string returnDate;

        BorrowingRecord(std::string readerId, std::string bookId, std::string borrowDate)
            : readerId(std::move(readerId)), bookId(std::move(bookId)),
              borrowDate(std::move(borrowDate)), returnDate("") {}

        [[nodiscard]] bool isReturned() const { return !returnDate.empty(); }

        [[nodiscard]] std::string toCsvLine() const; // 序列化成一行CSV，方便人直接读
        [[nodiscard]] std::string toDisplay() const;
    };

}