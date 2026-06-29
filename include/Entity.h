#pragma once
#include <memory>
#include <string>
namespace library
{
    /**
     * @brief 实体抽象类，这个类是作为所有类的基类（抽象类）存在的
     *
     * 这样做可以为所有的图书和读者提供统一的身份标识，方便存储
     */

    class Entity
    {
    protected:
        std::string id_;
        std::string name_;

    public:
        Entity(const std::string &id, const std::string &name)
            : id_(id), name_(name) {}

        virtual ~Entity() = default;
        // 这里我设计的禁用了拷贝构造和拷贝，改成移动了
        // 因为对于一个书，其实体应该是唯一的，所以拷贝不是很合理

        Entity(const Entity &) = delete;
        Entity &operator=(const Entity &) = delete;

        Entity(Entity &&) noexcept = default;
        Entity &operator=(Entity &&) noexcept = default;

        const std::string &getId() const { return id_; }
        const std::string &getName() const { return name_; }

        void setName(const std::string &name) { name_ = name; }

        virtual std::string getType() const = 0;

        virtual void display() const;
    };

}