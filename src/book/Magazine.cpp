#include "../../include/book/Magazine.h"

namespace library
{

    Magazine::Magazine(const std::string &id, const std::string &title,
                       const std::string &publisher, int issueNumber, const std::string &publishDate)
        : Book(id, title, "佚名", publisher), issueNumber_(issueNumber), publishDate_(publishDate) {}

    bool Magazine::isBorrowable() const
    {
        return false;
    }

    bool Magazine::borrowOut()
    {
        return false;
    }

    void Magazine::returnBack()
    {
        // 杂志无需归还
    }

    std::string Magazine::availabilityInfo() const
    {
        return "仅限馆内阅览";
    }

    std::string Magazine::getType() const
    {
        return "Magazine";
    }

}