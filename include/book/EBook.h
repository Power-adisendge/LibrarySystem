#pragma once

#include "Book.h"

namespace library
{

    /**
     * @brief 电子图书
     *
     * 可被无限次"借阅"（下载），不存在库存耗尽问题
     */
    class EBook : public Book
    {
    private:
        std::string format_;
        double fileSizeMb_;
        int downloadCount_ = 0;

    public:
        EBook(const std::string &id, const std::string &title,
              const std::string &author, const std::string &publisher,
              const std::string &format, double fileSizeMb);

        // ---- 基类纯虚接口 ----
        [[nodiscard]] bool isBorrowable() const override;

        bool borrowOut() override;

        void returnBack() override;

        [[nodiscard]] std::string availabilityInfo() const override;

        [[nodiscard]] int maxBorrowDays() const override;

        std::string getType() const override;

        const std::string &getFormat() const { return format_; }
        double getFileSizeMb() const { return fileSizeMb_; }
        int getDownloadCount() const { return downloadCount_; }
    };

}
