#include "../../include/reader/Reader.h"
#include <iostream>
#include <algorithm>

namespace library
{

    Reader::Reader(const std::string &id, const std::string &name)
        : Entity(id, name) {}

    BorrowResult Reader::borrow(const std::string &bookId)
    {
        if (hasBorrowed(bookId))
            return BorrowResult::AlreadyBorrowed;

        if (borrowedCount() >= maxBorrowLimit())
            return BorrowResult::LimitReached;

        borrowedBooks_.push_back(bookId);
        return BorrowResult::Success;
    }

    BorrowResult Reader::returnBook(const std::string &bookId)
    {
        // 在 vector 里找这本书
        auto it = std::find(borrowedBooks_.begin(), borrowedBooks_.end(), bookId);
        if (it == borrowedBooks_.end())
            return BorrowResult::NotBorrowed;

        borrowedBooks_.erase(it);
        return BorrowResult::Success;
    }

    bool Reader::hasBorrowed(const std::string &bookId) const
    {
        return std::find(borrowedBooks_.begin(), borrowedBooks_.end(), bookId) != borrowedBooks_.end();
    }

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

}