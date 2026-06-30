#include "../include/CsvStorage.h"

#include "../include/book/PhysicalBook.h"
#include "../include/book/EBook.h"
#include "../include/book/Magazine.h"
#include "../include/reader/Student.h"
#include "../include/reader/Teacher.h"

#include <fstream>
#include <sstream>

namespace library
{
    namespace
    {
        // 按逗号切分一行，保留尾部空字段（records 的未归还场景依赖它）
        std::vector<std::string> splitLine(const std::string &raw)
        {
            std::string line = raw;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            std::vector<std::string> fields;
            std::size_t start = 0;
            while (true)
            {
                std::size_t pos = line.find(',', start);
                if (pos == std::string::npos)
                {
                    fields.push_back(line.substr(start));
                    break;
                }
                fields.push_back(line.substr(start, pos - start));
                start = pos + 1;
            }
            return fields;
        }
    }

    // ============================ 保存 ============================

    void CsvStorage::saveBooks(const std::string &path, const std::vector<const Book *> &books)
    {
        std::ofstream out(path);
        for (const Book *b : books)
        {
            if (const auto *p = dynamic_cast<const PhysicalBook *>(b))
            {
                out << "PhysicalBook," << p->getId() << ',' << p->getName() << ','
                    << p->getAuthor() << ',' << p->getPublisher() << ','
                    << p->getTotalCopies() << '\n';
            }
            else if (const auto *e = dynamic_cast<const EBook *>(b))
            {
                out << "EBook," << e->getId() << ',' << e->getName() << ','
                    << e->getAuthor() << ',' << e->getPublisher() << ','
                    << e->getFormat() << ',' << e->getFileSizeMb() << '\n';
            }
            else if (const auto *m = dynamic_cast<const Magazine *>(b))
            {
                out << "Magazine," << m->getId() << ',' << m->getName() << ','
                    << m->getPublisher() << ',' << m->getIssueNumber() << ','
                    << m->getPublishDate() << '\n';
            }
            // 未知子类直接跳过，避免写出无法回读的脏行
        }
    }

    void CsvStorage::saveReaders(const std::string &path, const std::vector<const Reader *> &readers)
    {
        std::ofstream out(path);
        for (const Reader *r : readers)
        {
            out << r->getType() << ',' << r->getId() << ',' << r->getName() << '\n';
        }
    }

    void CsvStorage::saveRecords(const std::string &path, const std::vector<BorrowingRecord> &records)
    {
        std::ofstream out(path);
        for (const auto &rec : records)
            out << rec.toCsvLine() << '\n';
    }

    // ============================ 加载 ============================

    std::vector<std::unique_ptr<Book>> CsvStorage::loadBooks(const std::string &path)
    {
        std::vector<std::unique_ptr<Book>> books;
        std::ifstream in(path);
        if (!in)
            return books;

        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;
            const auto f = splitLine(line);
            const std::string &type = f[0];

            if (type == "PhysicalBook" && f.size() >= 6)
            {
                books.push_back(std::make_unique<PhysicalBook>(
                    f[1], f[2], f[3], f[4], std::stoi(f[5])));
            }
            else if (type == "EBook" && f.size() >= 7)
            {
                books.push_back(std::make_unique<EBook>(
                    f[1], f[2], f[3], f[4], f[5], std::stod(f[6])));
            }
            else if (type == "Magazine" && f.size() >= 6)
            {
                books.push_back(std::make_unique<Magazine>(
                    f[1], f[2], f[3], std::stoi(f[4]), f[5]));
            }
        }
        return books;
    }

    std::vector<std::unique_ptr<Reader>> CsvStorage::loadReaders(const std::string &path)
    {
        std::vector<std::unique_ptr<Reader>> readers;
        std::ifstream in(path);
        if (!in)
            return readers;

        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;
            const auto f = splitLine(line);
            if (f.size() < 3)
                continue;
            const std::string &type = f[0];

            if (type == "StudentReader")
                readers.push_back(std::make_unique<StudentReader>(f[1], f[2]));
            else if (type == "TeacherReader")
                readers.push_back(std::make_unique<TeacherReader>(f[1], f[2]));
        }
        return readers;
    }

    std::vector<BorrowingRecord> CsvStorage::loadRecords(const std::string &path)
    {
        std::vector<BorrowingRecord> records;
        std::ifstream in(path);
        if (!in)
            return records;

        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;
            const auto f = splitLine(line);
            if (f.size() < 4)
                continue;

            BorrowingRecord rec(f[0], f[1], f[2]);
            rec.returnDate = f[3];
            records.push_back(std::move(rec));
        }
        return records;
    }
}
