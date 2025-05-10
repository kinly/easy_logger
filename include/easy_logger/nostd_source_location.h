#include <cstdint>

#if __has_include(<source_location>) && __cplusplus >= 202002L

  #include <source_location>
  namespace nostd {
    using source_location = std::source_location;
  }

#else

namespace nostd {

namespace detail {

constexpr const char* move_end(const char* s) {
  return *s ? move_end(s + 1) : s;
}

constexpr const char* find_last_slash(const char* s, const char* last = nullptr) {
  return *s ? ((*s == '/' || *s == '\\') ? find_last_slash(s + 1, s + 1) : find_last_slash(s + 1, last)) : last;
}

constexpr const char* file_name(const char* path) {
  const char* last = find_last_slash(path);
  return last ? last : path;
}

}  // namespace detail

#if defined(__clang__) || defined(__GNUC__)
  #define NOSTD_FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
  #define NOSTD_FUNCTION_NAME __func__
#else
  #define NOSTD_FUNCTION_NAME "unknown"
#endif

#define NOSTD_FILE() nostd::detail::file_name(__FILE__)
#define NOSTD_FUNCTION() NOSTD_FUNCTION_NAME
#define NOSTD_LINE() static_cast<std::uint_least32_t>(__LINE__)
#define NOSTD_COLUMN() static_cast<std::uint_least32_t>(0)

class source_location {
 private:
  const char* _file;
  const char* _function;
  std::uint_least32_t _line;
  std::uint_least32_t _column;

 public:
  constexpr source_location(
    const char* file = NOSTD_FILE(),
    const char* function = NOSTD_FUNCTION(),
    std::uint_least32_t line = NOSTD_LINE(),
    std::uint_least32_t column = NOSTD_COLUMN()) noexcept
    : _file(file), _function(function), _line(line), _column(column) {}

  constexpr static source_location current(
    const char* file = NOSTD_FILE(),
    const char* function = NOSTD_FUNCTION(),
    std::uint_least32_t line = NOSTD_LINE(),
    std::uint_least32_t column = NOSTD_COLUMN()) noexcept {
    return source_location(file, function, line, column);
  }

  constexpr const char* file_name() const noexcept { return _file; }
  constexpr const char* function_name() const noexcept { return _function; }
  constexpr std::uint_least32_t line() const noexcept { return _line; }
  constexpr std::uint_least32_t column() const noexcept { return _column; }
};

}  // namespace nostd

#endif // fallback if <source_location> not available