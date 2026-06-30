#pragma once
#include "Reader.h"

namespace library
{

    class TeacherReader : public Reader
    {
    public:
        TeacherReader(const std::string &id, const std::string &name);

        [[nodiscard]] int maxBorrowLimit() const override;
        std::string getType() const override;
    };

}
