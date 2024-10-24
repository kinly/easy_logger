//
//  easy_logger.h
//  inlay
//
//  Created by Kinly on 2022/11/26.
//

#pragma once

#ifndef NOMINMAX
#    undef min
#    undef max
#endif

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/bundled/printf.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/hourly_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <sstream>
#include <memory>
#include <utility>
#include <filesystem>

#include "sinks/daily_folder_sink.h"

namespace util::logger {

    /// Struct with options for easy_logger
    struct easy_logger_options {
        std::string _key;      // The specific key of the logger (default "default")
        std::string _filename; // The filename with path (default "logs/log.log")
        std::string _folder;   // The parent folder
        union {
            uint16_t _mask;
            struct {
                uint16_t _daily_or_hourly : 2; // File will be named either _filename_YYYY-MM-DD.ext or _filename_YYYY-MM-DD_HH.ext
                uint16_t _daily_folder    : 1; // File location will be in _folder//YYYY-MM-DD//_filename.ext
                uint16_t _rotating_file   : 1; // File will be named _filename_{01~99}.ext and will be 1GB
                uint16_t _console         : 1; // Show logs in console (stdout)
            };
        };

        easy_logger_options() = default;

        easy_logger_options(std::string key, std::string filename, std::string folder, uint16_t mask)
            : _key(std::move(key))
            , _filename(std::move(filename))
            , _folder(std::move(folder))
            , _mask(mask) { }
    };

    /// Default values
    static constexpr char key_default[] = "default";
    static constexpr uint16_t mask_default = 39969;
    static easy_logger_options options_default = easy_logger_options(key_default, "logs/log.log", "", mask_default);

    class easy_logger_static
    {
    public:
        /// Initialize thread pool
        static void init()
        {
            constexpr std::size_t log_buffer_size = 32 * 1024; // 32kb
            spdlog::init_thread_pool(log_buffer_size, std::thread::hardware_concurrency());
        }

        // Will drop all register logger and shutdown
        static void shutdown()
        {
            spdlog::shutdown();
        }

        // Get the current logging level
        static auto level() -> decltype(spdlog::get_level())
        {
            return spdlog::get_level();
        }

        // Set the logging level
        static void set_level(spdlog::level::level_enum lvl)
        {
            spdlog::set_level(lvl);
        }

        // Check if we should log for a specific logging level
        static bool should_log(spdlog::level::level_enum lvl)
        {
            return spdlog::should_log(lvl);
        }

        // Flush on specific logging level
        static void set_flush_on(spdlog::level::level_enum lvl)
        {
            spdlog::flush_on(lvl);
        }

        /// Get the filename without the path
        static const char* get_shortname(const std::string& path)
        {
            if (path.empty())
                return path.data();

            const size_t pos = path.find_last_of("/\\");
            if (pos == std::string::npos)
                return path.data() + 0;
            return path.data() + (pos + 1);
        }
    };

    /// A log_line class to make logging easier
    class log_line
    {
    private:
        std::ostringstream _ss;

    public:
        log_line() noexcept { }

        template<class _Ty>
        log_line& operator << (const _Ty& src)
        {
            _ss << src;
            return *this;
        }
        std::string str() const
        {
            return _ss.str();
        }
    };

    /**
    * A log_stream class to make logging easier.
    * A logger is associated with a specific key.
    */
    template<const char* Key>
    class log_stream
    {
    private:
        spdlog::source_loc _loc;
        spdlog::level::level_enum _lvl = spdlog::level::info;
    public:
        log_stream(const spdlog::source_loc& loc, spdlog::level::level_enum lvl)
            : _loc(loc)
            , _lvl(lvl) { }
        bool operator ==(const log_line& ll) const
        {
            spdlog::get(Key)->log(_loc, _lvl, "{}", ll.str());
            return true;
        }
    };

    /**
     *  Easy_logger class to allow easier setup of loggers.
     */
    template<const char* Key>
    class easy_logger final : public easy_logger_static
    {
    public:
    private:
        std::atomic_bool _inited;
    public:
        static easy_logger<Key>& get()
        {
            static easy_logger<Key> easy_logger;
            return easy_logger;
        }

        /**
         * Initialize logger with options.
         * If we've already initialized, it just returns true.
         */
        bool init(const easy_logger_options& options)
        {
            if (_inited.load()) return true;

            try
            {
                // initialize spdlog
                constexpr std::size_t max_file_size = 1024 * 1024 * 1024; // 1G

                std::vector<spdlog::sink_ptr> sinks;
                if (options._daily_folder)
                {
                    sinks.push_back(std::make_shared<spdlog::sinks::daily_folder_sink_mt>(options._folder, options._filename));
                }

                if (options._daily_or_hourly == 1)
                {
                    sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(options._filename, 0, 2));
                }
                else if (options._daily_or_hourly == 2)
                {
                    sinks.push_back(std::make_shared<spdlog::sinks::hourly_file_sink_mt>(options._filename));
                }

                if (options._rotating_file)
                {
                    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(options._filename, max_file_size, 1024));
                }

                if (options._console)
                {
                    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
                }

#if defined(_DEBUG) && defined(WIN32)
                auto ms_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
                sinks.push_back(ms_sink);
#endif //  _DEBUG

                // register logger
                auto logger_ptr = std::make_shared<spdlog::logger>(options._key, sinks.begin(), sinks.end());
                spdlog::register_logger(logger_ptr);

                spdlog::get(Key)->set_pattern("[%Y-%m-%d %T.%e][%l][%@ %!][%P:%t]:%v");
                spdlog::get(Key)->flush_on(spdlog::level::warn);
                spdlog::get(Key)->set_level(spdlog::level::trace);
                spdlog::flush_every(std::chrono::seconds(3));
            }
            catch (std::exception_ptr e)
            {
                assert(false);
                return false;
            }

            _inited.store(true);
            return true;
        }

        template <typename... Args>
        /**
         * Log with source location and level.
         * Params: source location, log level, format string, and format string arguments.
         */
        void log(const spdlog::source_loc& loc, spdlog::level::level_enum lvl, const std::format_string<Args...> fmt, Args &&... args)
        {
            spdlog::get(Key)->log(loc, lvl, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        /**
         * Log with source location and level.
         * Params: source location, log level, format string, and format string arguments.
         */
        void print(const spdlog::source_loc& loc, spdlog::level::level_enum lvl, const char* fmt, const Args &... args)
        {
            spdlog::get(Key)->log(loc, lvl, fmt::sprintf(fmt, args...).c_str());
        }

    private:
        easy_logger() = default;
        ~easy_logger() = default;

        easy_logger(const easy_logger&) = delete;
        void operator=(const easy_logger&) = delete;
    };
} // namespace util::logger


// got short filename(exclude file directory)
#define __FILENAME__ (util::logger::easy_logger_static::get_shortname(__FILE__))

// use fmt lib, e.g. LOG_TRACE("warn log, {1}, {1}, {2}", 1, 2);
#define LOG_KEY_TRACE(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::trace))    util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace,    msg, ##__VA_ARGS__); };
#define LOG_KEY_DEBUG(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::debug))    util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::debug,    msg, ##__VA_ARGS__); };
#define LOG_KEY_INFO(key, msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::info))     util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::info,     msg, ##__VA_ARGS__); };
#define LOG_KEY_WARN(key, msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::warn))     util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::warn,     msg, ##__VA_ARGS__); };
#define LOG_KEY_ERROR(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::err))      util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::err,      msg, ##__VA_ARGS__); };
#define LOG_KEY_CRIT(key, msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::critical, msg, ##__VA_ARGS__); };

// use like sprintf, e.g. PRINT_TRACE("warn log, %d-%d", 1, 2);
#define PRINT_KEY_TRACE(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::trace))    util::logger::easy_logger<key>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace,    msg, ##__VA_ARGS__); };
#define PRINT_KEY_DEBUG(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::debug))    util::logger::easy_logger<key>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::debug,    msg, ##__VA_ARGS__); };
#define PRINT_KEY_INFO(key, msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::info))     util::logger::easy_logger<key>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::info,     msg, ##__VA_ARGS__); };
#define PRINT_KEY_WARN(key, msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::warn))     util::logger::easy_logger<key>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::warn,     msg, ##__VA_ARGS__); };
#define PRINT_KEY_ERROR(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::err))      util::logger::easy_logger<key>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::err,      msg, ##__VA_ARGS__); };
#define PRINT_KEY_CRIT(key, msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) util::logger::easy_logger<key>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::critical, msg, ##__VA_ARGS__); };

// operator << 优先级比 == 高...
#define __HIDE_STM_LOGGER(key, lvl) util::logger::easy_logger_static::should_log(lvl) && util::logger::log_stream<key>({__FILENAME__, __LINE__, __FUNCTION__}, lvl) == util::logger::log_line()
// use like stream, e.g. STM_TRACE() << "warn log: " << 1;
#define STM_KEY_TRACE(key) __HIDE_STM_LOGGER(key, spdlog::level::trace)
#define STM_KEY_DEBUG(key) __HIDE_STM_LOGGER(key, spdlog::level::debug)
#define STM_KEY_INFO(key)  __HIDE_STM_LOGGER(key, spdlog::level::info)
#define STM_KEY_WARN(key)  __HIDE_STM_LOGGER(key, spdlog::level::warn)
#define STM_KEY_ERROR(key) __HIDE_STM_LOGGER(key, spdlog::level::err)
#define STM_KEY_CRIT(key)  __HIDE_STM_LOGGER(key, spdlog::level::critical)

// use inner stream, e.g. ISTM_TRACE("warn log: " << 1 << "-" << 2)
#define ISTM_KEY_TRACE(key, msg) __HIDE_STM_LOGGER(key, spdlog::level::trace) << msg
#define ISTM_KEY_DEBUG(key, msg) __HIDE_STM_LOGGER(key, spdlog::level::debug) << msg
#define ISTM_KEY_INFO(key, msg)  __HIDE_STM_LOGGER(key, spdlog::level::info) << msg
#define ISTM_KEY_WARN(key, msg)  __HIDE_STM_LOGGER(key, spdlog::level::warn) << msg
#define ISTM_KEY_ERROR(key, msg) __HIDE_STM_LOGGER(key, spdlog::level::err) << msg
#define ISTM_KEY_CRIT(key, msg)  __HIDE_STM_LOGGER(key, spdlog::level::critical) << msg

// default
#define LOG_TRACE(msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::trace))    util::logger::easy_logger<util::logger::key_default>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace,    msg, ##__VA_ARGS__); };
#define LOG_DEBUG(msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::debug))    util::logger::easy_logger<util::logger::key_default>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::debug,    msg, ##__VA_ARGS__); };
#define LOG_INFO(msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::info))     util::logger::easy_logger<util::logger::key_default>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::info,     msg, ##__VA_ARGS__); };
#define LOG_WARN(msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::warn))     util::logger::easy_logger<util::logger::key_default>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::warn,     msg, ##__VA_ARGS__); };
#define LOG_ERROR(msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::err))      util::logger::easy_logger<util::logger::key_default>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::err,      msg, ##__VA_ARGS__); };
#define LOG_CRIT(msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) util::logger::easy_logger<util::logger::key_default>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::critical, msg, ##__VA_ARGS__); };

#define PRINT_TRACE(msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::trace))    util::logger::easy_logger<util::logger::key_default>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace,    msg, ##__VA_ARGS__); };
#define PRINT_DEBUG(msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::debug))    util::logger::easy_logger<util::logger::key_default>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::debug,    msg, ##__VA_ARGS__); };
#define PRINT_INFO(msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::info))     util::logger::easy_logger<util::logger::key_default>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::info,     msg, ##__VA_ARGS__); };
#define PRINT_WARN(msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::warn))     util::logger::easy_logger<util::logger::key_default>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::warn,     msg, ##__VA_ARGS__); };
#define PRINT_ERROR(msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::err))      util::logger::easy_logger<util::logger::key_default>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::err,      msg, ##__VA_ARGS__); };
#define PRINT_CRIT(msg,...)  { if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) util::logger::easy_logger<util::logger::key_default>::get().print({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::critical, msg, ##__VA_ARGS__); };

#define STM_TRACE() __HIDE_STM_LOGGER(util::logger::key_default, spdlog::level::trace)
#define STM_DEBUG() __HIDE_STM_LOGGER(util::logger::key_default, spdlog::level::debug)
#define STM_INFO()  __HIDE_STM_LOGGER(util::logger::key_default, spdlog::level::info)
#define STM_WARN()  __HIDE_STM_LOGGER(util::logger::key_default, spdlog::level::warn)
#define STM_ERROR() __HIDE_STM_LOGGER(util::logger::key_default, spdlog::level::err)
#define STM_CRIT()  __HIDE_STM_LOGGER(util::logger::key_default, spdlog::level::critical)
