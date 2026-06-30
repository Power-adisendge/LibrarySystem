#pragma once
#include <string>

namespace library
{
    // "YYYY-MM-DD" 之间差多少天，to 晚就是正数
    int daysBetween(const std::string &from, const std::string &to);

    // 把日期往后推 n 天
    std::string addDays(const std::string &date, int n);
}
