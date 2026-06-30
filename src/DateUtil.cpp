#include "../include/DateUtil.h"
#include <cstdio>

namespace library
{
    namespace
    {
        // Howard Hinnant 的算法：(y,m,d) 换成自纪元的天数序号
        long long daysFromCivil(int y, unsigned m, unsigned d)
        {
            y -= m <= 2;
            const int era = (y >= 0 ? y : y - 399) / 400;
            const unsigned yoe = static_cast<unsigned>(y - era * 400);
            const unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
            const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
            return era * 146097LL + static_cast<long long>(doe) - 719468;
        }

        // 反过来：天数序号还原成 (y,m,d)
        void civilFromDays(long long z, int &y, unsigned &m, unsigned &d)
        {
            z += 719468;
            const long long era = (z >= 0 ? z : z - 146096) / 146097;
            const unsigned doe = static_cast<unsigned>(z - era * 146097);
            const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
            y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
            const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
            const unsigned mp = (5 * doy + 2) / 153;
            d = doy - (153 * mp + 2) / 5 + 1;
            m = mp + (mp < 10 ? 3 : -9);
            y += (m <= 2);
        }

        long long serial(const std::string &s)
        {
            int y = 0, mo = 0, da = 0;
            std::sscanf(s.c_str(), "%d-%d-%d", &y, &mo, &da);
            return daysFromCivil(y, static_cast<unsigned>(mo), static_cast<unsigned>(da));
        }
    }

    int daysBetween(const std::string &from, const std::string &to)
    {
        return static_cast<int>(serial(to) - serial(from));
    }

    std::string addDays(const std::string &date, int n)
    {
        int y = 0;
        unsigned m = 0, d = 0;
        civilFromDays(serial(date) + n, y, m, d);
        char buf[11];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y,
                      static_cast<int>(m), static_cast<int>(d));
        return std::string(buf);
    }
}
