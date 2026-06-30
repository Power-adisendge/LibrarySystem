#pragma once
#include "Reader.h"

namespace library
{

    class StudentReader : public Reader
    {
    public:
        StudentReader(const std::string &id, const std::string &name);

        [[nodiscard]] int maxBorrowLimit() const override;
        std::string getType() const override;
    };

}