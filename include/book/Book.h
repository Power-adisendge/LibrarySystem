#pragma once

#include "../Entity.h"
#include <string>

namespace library
{
    /**
     * @brief 图书的基类，继承于实体抽象类
     *
     * 新增了作者、出版社之类的这些图书共有的特征，
     * 定义借阅相关的纯虚接口，由各类图书自行定义借阅策略
     */

    class Book : public Entity
    {
    protected:
        std::string author_;
        std::string publisher_;

    public:
        Book(const std::string &id, const std::string &title,
             const std::string &author, const std::string &publisher);

        ~Book() override = default;

        const std::string &getAuthor() const { return author_; }
        const std::string &getPublisher() const { return publisher_; }

        // 这个虚函数的设计是多态的纯虚接口（是否还能借阅-不同种类图书的库存逻辑不同）
        // 这里的nodiscard是为了保证这个“查询类”函数的返回值不会被丢弃
        [[nodiscard]] virtual bool isBorrowable() const = 0;

        virtual bool borrowOut() = 0;

        virtual void returnBack() = 0;

        // 看看当前可以借阅的数量（string形式）
        [[nodiscard]] virtual std::string availabilityInfo() const = 0;

        void display() const override;
    };
}
