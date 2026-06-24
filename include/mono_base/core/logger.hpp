#pragma once

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

#include <Windows.h>

namespace mono_base::log {

enum class Level : std::uint8_t {
    Trace, Debug, Info, Warn, Error, Fatal,
};

class Logger {
public:
    static Logger& instance();

    void init(std::wstring_view log_path);
    void shutdown();

    void set_level(Level level) { level_ = level; }

    void log(Level level, std::string_view tag, const char* fmt, ...);
    void logv(Level level, std::string_view tag, const char* fmt, va_list args);

private:
    Logger() = default;

    bool should_log(Level level) const {
        return static_cast<int>(level) >= static_cast<int>(level_);
    }

    static const char* level_name(Level level);
    static WORD console_color(Level level);
    void write_console(Level level, std::string_view line);
    void write_file(std::string_view line);

    std::mutex mutex_;
    std::ofstream file_;
    Level level_ = Level::Debug;
    bool console_attached_ = false;
};

inline Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

inline void Logger::init(std::wstring_view log_path) {
    std::lock_guard lock(mutex_);

    if (!file_.is_open()) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(log_path).parent_path(), ec);
        file_.open(log_path, std::ios::out | std::ios::trunc);
    }

    if (!console_attached_ && GetConsoleWindow() == nullptr) {
        if (AllocConsole()) {
            FILE* dummy = nullptr;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            SetConsoleTitleW(L"MonoBase");
            console_attached_ = true;
        }
    }
}

inline void Logger::shutdown() {
    std::lock_guard lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

inline const char* Logger::level_name(Level level) {
    switch (level) {
    case Level::Trace: return "TRACE";
    case Level::Debug: return "DEBUG";
    case Level::Info:  return "INFO ";
    case Level::Warn:  return "WARN ";
    case Level::Error: return "ERROR";
    case Level::Fatal: return "FATAL";
    }
    return "?????";
}

inline WORD Logger::console_color(Level level) {
    switch (level) {
    case Level::Trace: return FOREGROUND_INTENSITY;
    case Level::Debug: return FOREGROUND_BLUE | FOREGROUND_GREEN;
    case Level::Info:  return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case Level::Warn:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case Level::Error: return FOREGROUND_RED | FOREGROUND_INTENSITY;
    case Level::Fatal: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

inline void Logger::write_console(Level level, std::string_view line) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!out || out == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(out, &info)) {
        SetConsoleTextAttribute(out, console_color(level));
    }

    fwrite(line.data(), 1, line.size(), stdout);
    fputc('\n', stdout);
    fflush(stdout);

    if (GetConsoleScreenBufferInfo(out, &info)) {
        SetConsoleTextAttribute(out, info.wAttributes);
    }
}

inline void Logger::write_file(std::string_view line) {
    if (!file_.is_open()) {
        return;
    }
    file_.write(line.data(), static_cast<std::streamsize>(line.size()));
    file_.put('\n');
    file_.flush();
}

inline void Logger::logv(Level level, std::string_view tag, const char* fmt, va_list args) {
    if (!should_log(level)) {
        return;
    }

    char message[2048]{};
    vsnprintf(message, sizeof(message), fmt, args);

    const auto now = std::chrono::system_clock::now();
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
    localtime_s(&local, &tt);

    char line[2304]{};
    snprintf(line, sizeof(line),
        "[%02d:%02d:%02d.%03d] [%s] [%.*s] %s",
        local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(ms.count()),
        level_name(level), static_cast<int>(tag.size()), tag.data(), message);

    std::lock_guard lock(mutex_);
    write_file(line);
    write_console(level, line);
}

inline void Logger::log(Level level, std::string_view tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logv(level, tag, fmt, args);
    va_end(args);
}

#define LOG_TRACE(tag, ...) ::mono_base::log::Logger::instance().log(::mono_base::log::Level::Trace, tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...) ::mono_base::log::Logger::instance().log(::mono_base::log::Level::Debug, tag, __VA_ARGS__)
#define LOG_INFO(tag, ...)  ::mono_base::log::Logger::instance().log(::mono_base::log::Level::Info,  tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)  ::mono_base::log::Logger::instance().log(::mono_base::log::Level::Warn,  tag, __VA_ARGS__)
#define LOG_ERROR(tag, ...) ::mono_base::log::Logger::instance().log(::mono_base::log::Level::Error, tag, __VA_ARGS__)
#define LOG_FATAL(tag, ...) ::mono_base::log::Logger::instance().log(::mono_base::log::Level::Fatal, tag, __VA_ARGS__)

} // namespace mono_base::log
