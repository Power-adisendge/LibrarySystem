#include "../../include/book/EBook.h"

namespace library
{

    EBook::EBook(const std::string &id, const std::string &title,
                 const std::string &author, const std::string &publisher,
                 const std::string &format, double fileSizeMb)
        : Book(id, title, author, publisher), format_(format), fileSizeMb_(fileSizeMb) {}

    bool EBook::isBorrowable() const
    {
        return true;
    }

    bool EBook::borrowOut()
    {
        ++downloadCount_;
        return true;
    }

    void EBook::returnBack()
    {
        // 电子书无需归还
    }

    std::string EBook::availabilityInfo() const
    {
        return "电子资源（无限借阅）";
    }

    std::string EBook::getType() const
    {
        return "EBook";
    }

}