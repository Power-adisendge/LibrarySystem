#include "../../include/book/Book.h"
#include <iostream>

namespace library
{

    Book::Book(const std::string &id, const std::string &title,
               const std::string &author, const std::string &publisher)
        : Entity(id, title), author_(author), publisher_(publisher) {}

    void Book::display() const
    {
        std::cout << "[" << getType() << "] "
                  << getName()
                  << " 作者: " << author_
                  << " 出版社: " << publisher_
                  << " (" << availabilityInfo() << ")"
                  << std::endl;
    }

}