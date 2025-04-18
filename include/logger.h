//
//  easy_logger.h
//  inlay
//
//  Created by Kinly on 2022/11/26.
//

#pragma once

#ifndef NOMINMAX
#undef min
#undef max
#endif

#include <spdlog/async.h>
#include <spdlog/fmt/bundled/printf.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <sstream>
#include <thread>

/// spdlog wrap class
namespace util::logger {

class easy_logger_static {
 public:
  static void init() {
    constexpr std::size_t log_buffer_size = 1024 * 32;  // 32kb
    spdlog::init_thread_pool(log_buffer_size, std::thread::hardware_concurrency());
  }

  // will drop all register logger and shutdown
  static void shutdown() {
    spdlog::shutdown();
  }

  // spdlog static globally
  static auto level() -> decltype(spdlog::get_level()) {
    return spdlog::get_level();
  }

  static void set_level(spdlog::level::level_enum lvl) {
    spdlog::set_level(lvl);
  }

  static bool should_log(spdlog::level::level_enum lvl) {
    return spdlog::should_log(lvl);
  }

  static void set_flush_on(spdlog::level::level_enum lvl) {
    spdlog::flush_on(lvl);
  }

  static const char *get_shortname(const std::string &path) {
    if (path.empty())
      return path.data();

    const size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
      return path.data() + 0;
    return path.data() + (pos + 1);
  }
};

class easy_logger final : public easy_logger_static {
 public:
 private:
  std::atomic_bool _inited;

 public:
  static easy_logger &get() {
    static easy_logger easy_logger;
    return easy_logger;
  }

  bool init(std::string_view filename) {
    if (_inited.load())
      return true;

    try {
      // initialize spdlog
      std::vector<spdlog::sink_ptr> sinks;

      sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(filename.data(), 0, 2));

      // constexpr std::size_t max_file_size = 1024l * 1024 * 1024; // 1G
      // sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      //     filename.data(), max_file_size, 1024));

      sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
      // sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
#if !defined(WIN32) && !defined(NO_CONSOLE_LOG)
#endif

#if defined(_DEBUG) && defined(WIN32) && !defined(NO_CONSOLE_LOG)
      sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif  //  _DEBUG

      // register logger
      spdlog::set_default_logger(std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end()));

      // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
      // eg. [2024-07-15 11:15:54.345][debug][main.cpp:216][210852]:DEBUG log,
      // 1, 1, 2
      spdlog::set_pattern("%^[%Y-%m-%d %T.%e][%l][%@][%t]:%v%$");
      spdlog::flush_on(spdlog::level::warn);
      spdlog::set_level(spdlog::level::trace);
      spdlog::flush_every(std::chrono::seconds(3));

      spdlog::set_error_handler(
        [](const std::string &msg) { spdlog::log(spdlog::level::critical, "*** LOGGER ERROR ***: {}", msg); });

    } catch (const std::exception_ptr &e) {
      assert(false);
      return false;
    }

    _inited.store(true);

    return true;
  }

  /*
   * fmt 类型从 const char* 修改为 const std::format_string<args_tt...>
   * https://en.cppreference.com/w/cpp/utility/format/format
   * Since P2216R3, std::format does a compile-time check on the format string
   * (via the helper type std::format_string or std::wformat_string).
   * If it is found to be invalid for the types of the arguments to be
   * formatted, a compilation error will be emitted. If the format string cannot
   * be a compile-time constant, or the compile-time check needs to be avoided,
   * use std::vformat or std::runtime_format on fmt(since C++26) instead.
   */
  template <class... args_tt>
  static void log(const spdlog::source_loc &loc, spdlog::level::level_enum lvl,
    const std::format_string<args_tt...> fmt, args_tt &&...args) {
    spdlog::log(loc, lvl, fmt, std::forward<args_tt>(args)...);
  }

  template <typename... args_tt>
  static void print(
    const spdlog::source_loc &loc, spdlog::level::level_enum lvl, const char *fmt, const args_tt &...args) {
    spdlog::log(loc, lvl, fmt::sprintf(fmt, args...).c_str());
  }

  // template <typename... args_tt>
  // void stm(const spdlog::source_loc &loc, spdlog::level::level_enum lvl,
  //          args_tt &&...args) {
  //   constexpr auto arg_size = sizeof...(args);
  //   if constexpr (arg_size == 0)
  //     spdlog::log(loc, lvl, "");
  //   else if constexpr (arg_size == 1)
  //     spdlog::log(loc, lvl, "{}", std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 2)
  //     spdlog::log(loc, lvl, "{}{}", std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 3)
  //     spdlog::log(loc, lvl, "{}{}{}", std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 4)
  //     spdlog::log(loc, lvl, "{}{}{}{}", std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 5)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}", std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 6)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}", std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 7)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}",
  //     std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 8)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}",
  //     std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 9)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 10)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 11)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 12)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 13)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 14)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 15)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 16)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 17)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 18)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 19)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  //   else if constexpr (arg_size == 20)
  //     spdlog::log(loc, lvl, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
  //                 std::forward<args_tt>(args)...);
  // }

  // // via: https://stackoverflow.com/a/76429895/21686566
  // template <size_t count_vv>
  // static consteval auto make_format_string_placeholders()
  //     -> std::array<char, count_vv * 3 + 1> {
  //   return []<size_t... index_vv>(
  //              std::integer_sequence<size_t, index_vv...> &&) {
  //     return std::array<char, count_vv * 3 + 1>{(index_vv % 3 == 0   ? '{'
  //                                                : index_vv % 3 == 1 ? '}'
  //                                                                    : '
  //                                                                    ')...,
  //                                               '\0'};
  //   }(std::make_index_sequence<count_vv * 3>{});
  // }
  //
  // template <size_t count_vv>
  // struct format_string_placeholders {
  //   static constexpr auto arr = make_format_string_placeholders<count_vv>();
  //   static constexpr auto str = std::string_view{std::data(arr), count_vv *
  //   3};
  // };
  //
  // template <typename... args_tt>
  // static void stm(const spdlog::source_loc &loc, spdlog::level::level_enum
  // lvl,
  //                 args_tt &&...args) {
  //   spdlog::log(loc, lvl,
  //               std::format(format_string_placeholders<sizeof...(args)>::str,
  //                           std::forward<args_tt>(args)...));
  // }

  template <std::size_t count_vv>
  static consteval std::array<char, count_vv * 3> make_format_string_placeholders() {
    std::array<char, count_vv * 3> placeholders = {};
    for (std::size_t i = 0; i < count_vv; ++i) {
      placeholders[i * 3] = '{';
      placeholders[i * 3 + 1] = '}';
      if (i < count_vv - 1) {
        placeholders[i * 3 + 2] = ' ';
      }
    }
    return placeholders;
  }

  template <size_t count_vv>
  struct format_string_placeholders {
    static constexpr auto arr = make_format_string_placeholders<count_vv>();
    static constexpr std::size_t size = count_vv * 3 - (count_vv > 1 ? 1 : 0);
    static constexpr auto str = std::string_view{arr.data(), size};
  };

  template <typename... args_tt>
  static void stm(const spdlog::source_loc &loc, spdlog::level::level_enum lvl, args_tt &&...args) {
    constexpr std::size_t args_size = sizeof...(args);
    spdlog::log(loc, lvl, format_string_placeholders<args_size>::str, std::forward<args_tt>(args)...);
  }

 private:
  easy_logger() = default;
  ~easy_logger() = default;

  easy_logger(const easy_logger &) = delete;
  void operator=(const easy_logger &) = delete;
};
}  // namespace util::logger

// default
// use fmt lib, e.g. LOG_TRACE("warn log, {1}, {1}, {2}", 1, 2);
#define LOG_TRACE(msg, ...)                                                                                            \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::trace)) {                                          \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::log(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::trace, msg, ##__VA_ARGS__);           \
    }                                                                                                                  \
  };
#define LOG_DEBUG(msg, ...)                                                                                            \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::debug)) {                                          \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::log(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::debug, msg, ##__VA_ARGS__);           \
    }                                                                                                                  \
  };
#define LOG_INFO(msg, ...)                                                                                             \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::info)) {                                           \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::log(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::info, msg, ##__VA_ARGS__);            \
    }                                                                                                                  \
  };
#define LOG_WARN(msg, ...)                                                                                             \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::warn)) {                                           \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::log(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::warn, msg, ##__VA_ARGS__);            \
    }                                                                                                                  \
  };
#define LOG_ERROR(msg, ...)                                                                                            \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::err)) {                                            \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::log(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::err, msg, ##__VA_ARGS__);             \
    }                                                                                                                  \
  };
#define LOG_CRIT(msg, ...)                                                                                             \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) {                                       \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::log(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::critical, msg, ##__VA_ARGS__);        \
    }                                                                                                                  \
  };

// use like sprintf, e.g. PRINT_TRACE("warn log, %d-%d", 1, 2);
#define PRINT_TRACE(msg, ...)                                                                                          \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::trace)) {                                          \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::print(                                                                                \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::trace, msg, ##__VA_ARGS__);           \
    }                                                                                                                  \
  };

#define PRINT_DEBUG(msg, ...)                                                                                          \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::debug)) {                                          \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::print(                                                                                \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::debug, msg, ##__VA_ARGS__);           \
    }                                                                                                                  \
  };

#define PRINT_INFO(msg, ...)                                                                                           \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::info)) {                                           \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::print(                                                                                \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::info, msg, ##__VA_ARGS__);            \
    }                                                                                                                  \
  };

#define PRINT_WARN(msg, ...)                                                                                           \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::warn)) {                                           \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::print(                                                                                \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::warn, msg, ##__VA_ARGS__);            \
    }                                                                                                                  \
  };

#define PRINT_ERROR(msg, ...)                                                                                          \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::err)) {                                            \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::print(                                                                                \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::err, msg, ##__VA_ARGS__);             \
    }                                                                                                                  \
  };

#define PRINT_CRIT(msg, ...)                                                                                           \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) {                                       \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::print(                                                                                \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::critical, msg, ##__VA_ARGS__);        \
    }                                                                                                                  \
  };

// use like stringstream, e.g. STM_TRACE("warn log ", 1, " ", 2); => warn log 1 2
#define STM_TRACE(...)                                                                                                 \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::trace)) {                                          \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::stm(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::trace, ##__VA_ARGS__);                \
    }                                                                                                                  \
  }

#define STM_DEBUG(...)                                                                                                 \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::debug)) {                                          \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::stm(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::debug, ##__VA_ARGS__);                \
    }                                                                                                                  \
  }

#define STM_INFO(...)                                                                                                  \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::info)) {                                           \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::stm(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::info, ##__VA_ARGS__);                 \
    }                                                                                                                  \
  }

#define STM_WARN(msg, ...)                                                                                             \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::warn)) {                                           \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::stm(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::warn, ##__VA_ARGS__);                 \
    }                                                                                                                  \
  }

#define STM_ERROR(msg, ...)                                                                                            \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::err)) {                                            \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::stm(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::err, ##__VA_ARGS__);                  \
    }                                                                                                                  \
  }

#define STM_CRIT(msg, ...)                                                                                             \
  {                                                                                                                    \
    if (util::logger::easy_logger_static::should_log(spdlog::level::critical)) {                                       \
      constexpr nostd::source_location ns_sl =                                                                         \
        nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __builtin_COLUMN()); \
      util::logger::easy_logger::stm(                                                                                  \
        {ns_sl.file_name(), ns_sl.line(), ns_sl.function_name()}, spdlog::level::critical, ##__VA_ARGS__);             \
    }                                                                                                                  \
  }
