#pragma once
#include <cstdint>
#include <source_location>

namespace nostd {

// https://stackoverflow.com/questions/31050113/how-to-extract-the-source-filename-without-path-and-suffix-at-compile-time
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
namespace __ {
constexpr const char *move_end(const char *source) {
  return *source ? move_end(source + 1) : source;
}
constexpr bool is_exist_slant(const char *source) {
  return *source == '/' || *source == '\\' ? true : (*source ? is_exist_slant(source + 1) : false);
}
constexpr const char *remove_slant(const char *source) {
  return *source == '/' || *source == '\\' ? (source + 1) : remove_slant(source - 1);
}
constexpr const char *file_name(const char *source) {
  return is_exist_slant(source) ? remove_slant(move_end(source)) : source;
}
};  // end namespace __

// https://stackoverflow.com/questions/57137351/line-is-not-constexpr-in-msvc
#define _LINE_CONCATENATE_(a, b) a##b
#define _LINE_CONCATENATE(a, b) _LINE_CONCATENATE_(a, b)
#define CONSTEXPR_LINE uint_least32_t(_LINE_CONCATENATE(__LINE__, U))

#define __builtin_FILE() (nostd::__::file_name(__FILE__))
#define __builtin_FUNCTION() ("unknown")
#define __builtin_LINE() (CONSTEXPR_LINE)
#define __builtin_COLUMN() (0)
#endif

struct source_location {
 private:
  const char *_filename;
  const char *_function_name;
  const uint_least32_t _line;
  const uint_least32_t _column;

 private:
  constexpr source_location(
    const char *file, const char *function, const uint_least32_t line, const uint_least32_t column)
      : _filename(file), _function_name(function), _line(line), _column(column) {}

 public:
  static constexpr source_location current(const char *file = __builtin_FILE(),
    const char *function = __builtin_FUNCTION(), const uint_least32_t line = __builtin_LINE(),
    const uint_least32_t column = __builtin_COLUMN()) noexcept {
    return source_location{file, function, line, column};
  }
  source_location(const source_location &) = default;
  source_location(source_location &&) = default;

  constexpr const char *file_name() const noexcept {
    return _filename;
  }

  constexpr const char *function_name() const noexcept {
    return _function_name;
  }

  constexpr uint_least32_t line() const noexcept {
    return _line;
  }

  constexpr std::uint_least32_t column() const noexcept {
    return _column;
  }

};  // end namespace source_location
};  // end namespace nostd

/*
 *
constexpr auto xx = nostd::__::file_name(__FILE__);
// 必须有 constexpr 对象定义的地方
constexpr nostd::source_location sl =
    nostd::source_location::current(__builtin_FILE(), __builtin_FUNCTION(),
                                    __builtin_LINE(), __builtin_COLUMN());

 */