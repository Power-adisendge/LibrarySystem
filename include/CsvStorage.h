// 可持久化部分

#pragma once

#include "book/Book.h"
#include "reader/Reader.h"
#include "BorrowingRecord.h"

#include <memory>
#include <string>
#include <vector>

namespace library
{
    /**
     * @brief 唯一的持久化类：负责领域对象和CSV文件的双向转换
     */
    class CsvStorage
    {
    public:
        // ---- 保存：调用方提供对象，本类只负责格式化写出 ----
        static void saveBooks(const std::string &path, const std::vector<const Book *> &books);
        static void saveReaders(const std::string &path, const std::vector<const Reader *> &readers);
        static void saveRecords(const std::string &path, const std::vector<BorrowingRecord> &records);

        // 文件不存在时返回空容器（视为还没有任何数据，不抛异常）
        [[nodiscard]] static std::vector<std::unique_ptr<Book>> loadBooks(const std::string &path);
        [[nodiscard]] static std::vector<std::unique_ptr<Reader>> loadReaders(const std::string &path);
        [[nodiscard]] static std::vector<BorrowingRecord> loadRecords(const std::string &path);
    };
}
