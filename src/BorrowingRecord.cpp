#include "../include/BorrowingRecord.h"
namespace library
{
    // 序列化成一行 CSV："readerId,bookId,borrowDate,returnDate"
    // 未归还时 returnDate 为空，于是结尾是个空字段，解析时据此判断

    std::string BorrowingRecord::toCsvLine() const
    {
        return readerId + "," + bookId + "," +
               borrowDate + "," + returnDate;
    }

    std::string BorrowingRecord::toDisplay() const
    {
        std::string s = "[" + borrowDate + "] Reader " + readerId +
                        " 借阅了书 " + bookId;
        if (isReturned())
            s += ", 归还于 " + returnDate;
        else
            s += " (未归还)";
        return s;
    }
}