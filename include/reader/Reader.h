#pragma once

#include "../Entity.h"
#include <string>
#include <vector>

namespace library
{

    // 借阅操作的结果
    enum class BorrowResult
    {
        Success,         // 成功
        LimitReached,    // 已达借阅上限
        AlreadyBorrowed, // 这本书已经借了
        NotBorrowed      // 还书时发现没借过
    };

    class Reader : public Entity
    {
    private:
        std::vector<std::string> borrowedBooks_; // 当前借出书籍的id

    public:
        Reader(const std::string &id, const std::string &name);
        ~Reader() override = default;

        // 借阅上限由子类决定（实现多态）
        [[nodiscard]] virtual int maxBorrowLimit() const = 0;
        // 最长借书期限（天）
        [[nodiscard]] virtual int maxBorrowDays() const = 0;
        // 每天超期罚款（元）
        [[nodiscard]] virtual double finePerDay() const = 0;

        BorrowResult borrow(const std::string &bookId);
        BorrowResult returnBook(const std::string &bookId);

        [[nodiscard]] bool hasBorrowed(const std::string &bookId) const;
        [[nodiscard]] int borrowedCount() const
        {
            return static_cast<int>(borrowedBooks_.size());
        }
        const std::vector<std::string> &getBorrowedBooks() const
        {
            return borrowedBooks_;
        }

        void display() const override;
    };

}
