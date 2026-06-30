#include "../include/reader/Student.h"
#include "../include/reader/Teacher.h"
#include <iostream>

using namespace library;

// 把 enum 结果转成可读文字，这样测试就挺方便的
const char *resultText(BorrowResult r)
{
    switch (r)
    {
    case BorrowResult::Success:
        return "成功";
    case BorrowResult::LimitReached:
        return "已达上限";
    case BorrowResult::AlreadyBorrowed:
        return "重复借阅";
    case BorrowResult::NotBorrowed:
        return "未借过此书";
    }
    return "未知";
}

int main()
{
    StudentReader stu("S001", "小明");
    stu.display();

    // 借 6 本，第 6 本应触发上限（学生上限 5）
    for (int i = 1; i <= 6; ++i)
    {
        std::string bookId = "B00" + std::to_string(i);
        BorrowResult r = stu.borrow(bookId);
        std::cout << "借 " << bookId << " -> " << resultText(r) << std::endl;
    }

    // 重复借同一本
    std::cout << "再借 B001 -> " << resultText(stu.borrow("B001")) << std::endl;

    stu.display();

    // 还书
    std::cout << "还 B003 -> " << resultText(stu.returnBook("B003")) << std::endl;
    std::cout << "还 B999 -> " << resultText(stu.returnBook("B999")) << std::endl;
    stu.display();

    std::cout << "----" << std::endl;

    TeacherReader tea("T001", "王老师");
    tea.display(); // 上限应为 20

    return 0;
}