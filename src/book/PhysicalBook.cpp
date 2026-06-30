#include "../../include/book/PhysicalBook.h"

namespace library
{

    PhysicalBook::PhysicalBook(const std::string &id, const std::string &title,
                               const std::string &author, const std::string &publisher, int totalCopies)
        : Book(id, title, author, publisher), totalCopies_(totalCopies), availableCopies_(totalCopies) {}

    bool PhysicalBook::isBorrowable() const
    {
        return availableCopies_ > 0;
    }

    bool PhysicalBook::borrowOut()
    {
        if (!isBorrowable())
            return false;
        --availableCopies_;
        return true;
    }

    void PhysicalBook::returnBack()
    {
        if (availableCopies_ < totalCopies_)
            ++availableCopies_;
    }

    std::string PhysicalBook::availabilityInfo() const
    {
        return std::to_string(availableCopies_) + "/" +
               std::to_string(totalCopies_) + " 可借 · 借期30天";
    }

    int PhysicalBook::maxBorrowDays() const
    {
        return 30;
    }

    std::string PhysicalBook::getType() const
    {
        return "PhysicalBook";
    }

} // namespace library