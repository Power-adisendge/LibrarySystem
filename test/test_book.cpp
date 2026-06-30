#include "include/book/PhysicalBook.h"
#include "include/book/EBook.h"
#include "include/book/Magazine.h"
#include <iostream>
#include <vector>
#include <memory>

using namespace library;

int main()
{
    std::vector<std::unique_ptr<Book>> books;
    books.push_back(std::make_unique<PhysicalBook>(
        "B001", "C++ Primer", "Lippman", "电子工业出版社", 3));
    books.push_back(std::make_unique<EBook>(
        "E001", "Effective C++", "Meyers", "机械工业出版社", "PDF", 8.5));
    books.push_back(std::make_unique<Magazine>(
        "M001", "读者", "读者出版社", 2024, "2024-01-15"));

    for (const auto &book : books)
    {
        std::cout << "[" << book->getType() << "] "
                  << book->getName()
                  << " - " << book->availabilityInfo() << std::endl;

        if (book->borrowOut())
        {
            std::cout << "  借阅成功 -> " << book->availabilityInfo() << std::endl;
        }
        else
        {
            std::cout << "  借阅失败" << std::endl;
        }
    }
    return 0;
}