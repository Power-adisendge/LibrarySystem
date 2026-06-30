#include "../../include/reader/Student.h"

namespace library
{

    StudentReader::StudentReader(const std::string &id, const std::string &name)
        : Reader(id, name) {}

    int StudentReader::maxBorrowLimit() const
    {
        return 5;
    }

    std::string StudentReader::getType() const
    {
        return "StudentReader";
    }

}