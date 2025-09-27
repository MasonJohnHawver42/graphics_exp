#include "log.h"
#include "types.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>

namespace cm
{

    // Thread-local buffer for logging
    std::ostringstream &LogStream::buffer()
    {
        thread_local static std::ostringstream buf;
        return buf;
    }

    // LogStream constructor
    LogStream::LogStream(u8 error, const char *file, i32 line)
        : error_(error), file_(file), line_(line)
    {
        // Reset the thread-local buffer
        buffer().str("");
        buffer().clear();
    }

    // LogStream destructor
    LogStream::~LogStream()
    {
        _log_internal(error_, file_, line_, "%s", buffer().str().c_str());
    }

    void _log_internal(u8 error, const char *file, i32 line, const char *fmt, ...)
    {
        const int filePad = 9; // [<file>] pasrt should always be pad + 2 chars long
        const int linePad = 3; // (<line>) pasrt should always be pad + 2 chars long hint use strlen

        if (error)
        {
            printf("[ERROR] ");
        }

        std::ostream &output_stream = (error ? std::cerr : std::cout);

        int lineLen = snprintf(nullptr, 0, "%d", line);

        int file_total_pad = std::max(0, filePad - (int)strlen(file));
        int line_total_pad = std::max(0, linePad - lineLen);

        std::string file_pad_left = std::string(file_total_pad / 2, ' ');
        std::string line_pad_left = std::string(line_total_pad / 2, ' ');

        std::string file_pad_right = std::string(((file_total_pad % 2 == 0) ? file_total_pad / 2 : (file_total_pad / 2) + 1), ' ');
        std::string line_pad_right = std::string(((line_total_pad % 2 == 0) ? line_total_pad / 2 : (line_total_pad / 2) + 1), ' ');

        output_stream << "[" << file_pad_left << file << file_pad_right << "] (" << line_pad_left << line_pad_right << line << ") : ";

        // Handle variable arguments
        va_list args;
        va_start(args, fmt);
        vfprintf((error ? stderr : stdout), fmt, args);
        va_end(args);

        output_stream << "\n";
    }

    #ifdef DEBUG

    TracedException::TracedException(const std::string& message)
        : std::runtime_error(message), trace_(cpptrace::generate_trace()) {}

    const char* TracedException::what() const noexcept {
        if (what_str_.empty()) {
            what_str_ = std::runtime_error::what();
            what_str_ += "\n\n";
            what_str_ += trace_.to_string(true);
        }
        return what_str_.c_str();
    }

    const cpptrace::stacktrace& TracedException::trace() const noexcept {
        return trace_;
    }

    #endif
} // namespace cm