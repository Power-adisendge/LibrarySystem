#pragma once
#include "Book.h"

namespace library
{

    /**
     * @brief 实体纸质图书
     *
     * 有真实库存，借一本少一本，还一本多一本
     */
    class PhysicalBook : public Book
    {
    private:
        int totalCopies_;
        int availableCopies_;

    public:
        PhysicalBook(const std::string &id, const std::string &title,
                     const std::string &author, const std::string &publisher, int totalCopies);

        // ---- 实现基类纯虚接口 ----
        [[nodiscard]] bool isBorrowable() const override;

        bool borrowOut() override;

        void returnBack() override;

        [[nodiscard]] std::string availabilityInfo() const override;

        [[nodiscard]] int maxBorrowDays() const override;

        std::string getType() const override;

        int getTotalCopies() const { return totalCopies_; }
        int getAvailableCopies() const { return availableCopies_; }
    };

}
