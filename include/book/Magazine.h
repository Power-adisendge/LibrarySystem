#pragma once

#include "Book.h"

namespace library
{

    /**
     * @brief 杂志期刊
     *
     * 按期管理，通常仅限馆内阅览，不外借
     */
    class Magazine : public Book
    {
    private:
        int issueNumber_;
        std::string publishDate_;

    public:
        Magazine(const std::string &id, const std::string &title,
                 const std::string &publisher, int issueNumber, const std::string &publishDate);

        // ---- 基类纯虚接口 ----
        [[nodiscard]] bool isBorrowable() const override;

        bool borrowOut() override;

        void returnBack() override;

        [[nodiscard]] std::string availabilityInfo() const override;

        std::string getType() const override;

        int getIssueNumber() const { return issueNumber_; }
        const std::string &getPublishDate() const { return publishDate_; }
    };

}
