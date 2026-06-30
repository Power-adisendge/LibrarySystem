#pragma once
#include <stdexcept>
#include <string>

namespace library
{

    /**
     * @brief 图书馆异常体系的基类
     *
     * 继承自 runtime_error，统一异常来源，
     * 方便调用方catch (const LibraryException&) 一网打尽
     */
    class LibraryException : public std::runtime_error
    {
    public:
        explicit LibraryException(const std::string &message)
            : std::runtime_error(message) {}
    };

    // 按ID 找书找不到时抛出
    class BookNotFoundException : public LibraryException
    {
    public:
        explicit BookNotFoundException(const std::string &bookId)
            : LibraryException("Book not found: \"" + bookId + "\"") {}
    };

    // 按 ID 找读者找不到时抛出
    class ReaderNotFoundException : public LibraryException
    {
    public:
        explicit ReaderNotFoundException(const std::string &readerId)
            : LibraryException("Reader not found: \"" + readerId + "\"") {}
    };

}