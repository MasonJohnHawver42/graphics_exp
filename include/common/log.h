#ifndef LOG_H
#define LOG_H

#include <cstring>
#include <cstdio>
#include <sstream>
#include <string>

#ifdef DEBUG

#include <stdexcept>
#include <cpptrace/cpptrace.hpp>

#endif

#include "types.h"
// #include <stacktrace>

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

namespace cm
{

    class LogStream
    {
    public:
        LogStream(u8 error, const char *file, i32 line);
        ~LogStream();

        template <typename T>
        LogStream &&operator<<(T &&value) &&
        {
            buffer() << std::forward<T>(value);
            return std::move(*this);
        }

    private:
        static std::ostringstream &buffer();

        u8 error_;
        const char *file_;
        i32 line_;
    };


    void _log_internal(u8 error, const char *file, i32 line, const char *fmt, ...);

    #ifdef DEBUG

    class TracedException : public std::runtime_error {
    public:
        explicit TracedException(const std::string& message);
    
        /// Returns a message with the original error and stack trace
        const char* what() const noexcept override;
    
        /// Access the raw stack trace object if needed
        const cpptrace::stacktrace& trace() const noexcept;
    
    private:
        cpptrace::stacktrace trace_;
        mutable std::string what_str_;
    }; 

    #endif

} // namespace cm

#ifdef DEBUG
// Release mode: throw exception if condition fails
#include <stdexcept>
#define ASSERT(condition, message) do { if (!(condition)) { throw cm::TracedException(message); } } while (false)
#else
// Debug mode: use assert, prints message and aborts
#include <cassert>
#define ASSERT(condition, message) assert((condition) && (message))
#endif

#define LOG_INFO(fmt, ...) cm::_log_internal(0, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) cm::_log_internal(1, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)

#define STREAM_INFO cm::LogStream(0, __FILENAME__, __LINE__)
#define STREAM_ERROR cm::LogStream(1, __FILENAME__, __LINE__)

#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) cm::_log_internal(0, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
#define STREAM_DEBUG cm::LogStream(0, __FILENAME__, __LINE__)
#else
class NullLogStream
{
public:
    template <typename T>
    NullLogStream &operator<<(T &&) { return *this; }
};
#define LOG_DEBUG(fmt, ...) ((void)0) // do nothing finish
#define STREAM_DEBUG NullLogStream()
#endif

#endif // LOG_H