#include "../../include/reader/Teacher.h"

namespace library
{

    TeacherReader::TeacherReader(const std::string &id, const std::string &name)
        : Reader(id, name) {}

    int TeacherReader::maxBorrowLimit() const
    {
        return 20;
    }

    std::string TeacherReader::getType() const
    {
        return "TeacherReader";
    }

}