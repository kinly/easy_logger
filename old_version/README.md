
# 2024-10-24 bug fixed:
- c++20 编译问题：'std::basic_format_string<char,const int &>::basic_format_string': call to immediate function is not a constant expression
- https://www.reddit.com/r/cpp_questions/comments/1aohxk8/stdformat_error_call_to_consteval_function/
![image](https://github.com/user-attachments/assets/b9bc2ffd-bad6-485e-bc97-66db69699eb7)
- https://en.cppreference.com/w/cpp/utility/format/format

# easy_logger

- 基于 spdlog 的易用封装
- 参考 daily_file_sink 新增了 daily_folder_sink，按天拆分目录

# pre
[spdlog](https://github.com/gabime/spdlog) 是一个很不错的日志库，自定义程度很高，之前就用过，只是有同事封装过了就直接用了，没有细看很多，前面看到[老顾头博客](https://gqw.github.io/posts/c++/logger_analyisis/)更新了相关的博文，看了下他在github分享的[wrapper封装](https://github.com/gqw/spdlog_wrapper)，就起了兴致在这个基础上扩展一下，因为用的是 mac + vscode + clang 环境，有些东西也值得记录一下

# 封装
- 支持基础的 daily_file, rotate, hourly_file
``` cpp
if (options._daily_or_hourly == 1) {
    sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(options._filename, 0, 2));
}
else if (options._daily_or_hourly == 2) {
    sinks.push_back(std::make_shared<spdlog::sinks::hourly_file_sink_mt>(options._filename));
}
if (options._rotating_file) {
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(options._filename, max_file_size, 1024));
}
```
- 增加 key 的方式，支持多个分文件的日志，常见的用法里会把异步的行为单独放一个日志文件，比如：网络基础日志 net.log、三方sdk日志 sdk.log....
``` cpp

template<const char* Key>
class easy_logger final : public easy_logger_static
{...};

#define LOG_TRACE(key, msg,...) { if (util::logger::easy_logger_static::should_log(spdlog::level::trace))    util::logger::easy_logger<key>::get().log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace,    msg, ##__VA_ARGS__); };
```
- 扩展支持 daily_folder (日常使用上希望每天一个日志目录，这样想拉日志的时候比较容易打包)，基本是参考 daily_file_sink 写的
``` cpp
template<typename Mutex, typename FolderNameCalc = daily_foldername_calculator>
class daily_folder_sink final : public base_sink<Mutex>
{...}
```

- future：支持rotating的daily日志，日志还是按天分比较合理，但也不希望一个日志文件过大

# 需要注意的地方
- mac + vscode + clang 的环境，编译时遇到了一些问题
- 没有直接用 xcode 是觉得之后做这种小的开发 vscode 更合适，也需要配置好本地的 vscode c++ 开发环境
- 主要依赖的一些 extensions：
    - c/c++
    - c/c++ project generator
    - c/c++ themes
    - code runner
    - codelldb
    - makefile tools
- 开始时候 easy_logger.h 头文件依赖是这么写的
``` cpp
#include <sstream>
#include <memory>
#include <utility>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/bundled/printf.h>
```
- 编译遇到 link error
``` bash
clang++ -std=c++17 -Wall -Wextra -g -Iinclude -Iinclude/sinks -I3rd -I3rd/spdlog -I3rd/spdlog/fmt -I3rd/spdlog/fmt/bundled -I3rd/spdlog/sinks -I3rd/spdlog/details -I3rd/spdlog/cfg -o output/main src/main.o
Undefined symbols for architecture x86_64:
  "fmt::v9::format_error::~format_error()", referenced from:
      fmt::v9::detail::adjust_precision(int&, int) in main.o
      void fmt::v9::detail::vprintf<char, fmt::v9::basic_printf_context<fmt::v9::appender, char>>(fmt::v9::detail::buffer<char>&, fmt::v9::basic_string_view<char>, fmt::v9::basic_format_args<fmt::v9::basic_printf_context<fmt::v9::appender, char>>) in main.o
      int fmt::v9::detail::parse_header<char, void fmt::v9::detail::vprintf<char, fmt::v9::basic_printf_context<fmt::v9::appender, char>>(fmt::v9::detail::buffer<char>&, fmt::v9::basic_string_view<char>, fmt::v9::basic_format_args<fmt::v9::basic_printf_context<fmt::v9::appender, char>>)::'lambda'(int)>(char const*&, char const*, fmt::v9::basic_format_specs<char>&, fmt::v9::basic_printf_context<fmt::v9::appender, char>) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<int, 0>(int) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<unsigned int, 0>(unsigned int) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<long long, 0>(long long) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<unsigned long long, 0>(unsigned long long) in main.o
      ...
  "fmt::v9::format_system_error(fmt::v9::detail::buffer<char>&, int, char const*)", referenced from:
      spdlog::spdlog_ex::spdlog_ex(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, int) in main.o
  "fmt::v9::detail::assert_fail(char const*, int, char const*)", referenced from:
      fmt::v9::detail::format_decimal_result<char*> fmt::v9::detail::format_decimal<char, unsigned int>(char*, unsigned int, int) in main.o
      std::__1::make_unsigned<long>::type fmt::v9::detail::to_unsigned<long>(long) in main.o
      fmt::v9::detail::format_decimal_result<char*> fmt::v9::detail::format_decimal<char, unsigned long long>(char*, unsigned long long, int) in main.o
      fmt::v9::detail::format_decimal_result<char*> fmt::v9::detail::format_decimal<char, unsigned __int128>(char*, unsigned __int128, int) in main.o
      std::__1::make_unsigned<int>::type fmt::v9::detail::to_unsigned<int>(int) in main.o
      fmt::v9::appender fmt::v9::detail::write_exponent<char, fmt::v9::appender>(int, fmt::v9::appender) in main.o
      int fmt::v9::detail::snprintf_float<long double>(long double, int, fmt::v9::detail::float_specs, fmt::v9::detail::buffer<char>&) in main.o
      ...
  "fmt::v9::detail::is_printable(unsigned int)", referenced from:
      fmt::v9::detail::needs_escape(unsigned int) in main.o
  "char fmt::v9::detail::decimal_point_impl<char>(fmt::v9::detail::locale_ref)", referenced from:
      char fmt::v9::detail::decimal_point<char>(fmt::v9::detail::locale_ref) in main.o
  "fmt::v9::detail::thousands_sep_result<char> fmt::v9::detail::thousands_sep_impl<char>(fmt::v9::detail::locale_ref)", referenced from:
      fmt::v9::detail::thousands_sep_result<char> fmt::v9::detail::thousands_sep<char>(fmt::v9::detail::locale_ref) in main.o
  "fmt::v9::detail::throw_format_error(char const*)", referenced from:
      fmt::v9::detail::error_handler::on_error(char const*) in main.o
      fmt::v9::appender fmt::v9::detail::write_int_noinline<char, fmt::v9::appender, unsigned int>(fmt::v9::appender, fmt::v9::detail::write_int_arg<unsigned int>, fmt::v9::basic_format_specs<char> const&, fmt::v9::detail::locale_ref) in main.o
      fmt::v9::appender fmt::v9::detail::write<char, fmt::v9::appender, long double, 0>(fmt::v9::appender, long double, fmt::v9::basic_format_specs<char>, fmt::v9::detail::locale_ref) in main.o
      fmt::v9::appender fmt::v9::detail::write<char, fmt::v9::appender>(fmt::v9::appender, char const*) in main.o
      fmt::v9::detail::fill_t<char>::operator=(fmt::v9::basic_string_view<char>) in main.o
      fmt::v9::appender fmt::v9::detail::write_int_noinline<char, fmt::v9::appender, unsigned long long>(fmt::v9::appender, fmt::v9::detail::write_int_arg<unsigned long long>, fmt::v9::basic_format_specs<char> const&, fmt::v9::detail::locale_ref) in main.o
      fmt::v9::appender fmt::v9::detail::write_int_noinline<char, fmt::v9::appender, unsigned __int128>(fmt::v9::appender, fmt::v9::detail::write_int_arg<unsigned __int128>, fmt::v9::basic_format_specs<char> const&, fmt::v9::detail::locale_ref) in main.o
      ...
  "fmt::v9::detail::dragonbox::decimal_fp<double> fmt::v9::detail::dragonbox::to_decimal<double>(double)", referenced from:
      fmt::v9::appender fmt::v9::detail::write<char, fmt::v9::appender, double, 0>(fmt::v9::appender, double) in main.o
      int fmt::v9::detail::format_float<double>(double, int, fmt::v9::detail::float_specs, fmt::v9::detail::buffer<char>&) in main.o
  "fmt::v9::detail::dragonbox::decimal_fp<float> fmt::v9::detail::dragonbox::to_decimal<float>(float)", referenced from:
      fmt::v9::appender fmt::v9::detail::write<char, fmt::v9::appender, float, 0>(fmt::v9::appender, float) in main.o
      int fmt::v9::detail::format_float<double>(double, int, fmt::v9::detail::float_specs, fmt::v9::detail::buffer<char>&) in main.o
  "fmt::v9::vformat(fmt::v9::basic_string_view<char>, fmt::v9::basic_format_args<fmt::v9::basic_format_context<fmt::v9::appender, char>>)", referenced from:
      spdlog::logger::sink_it_(spdlog::details::log_msg const&) in main.o
      spdlog::sinks::daily_foldername_calculator::calc_foldername(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, tm const&) in main.o
      spdlog::sinks::daily_filename_calculator::calc_filename(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, tm const&) in main.o
      spdlog::sinks::hourly_filename_calculator::calc_filename(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, tm const&) in main.o
      spdlog::sinks::rotating_file_sink<std::__1::mutex>::calc_filename(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, unsigned long) in main.o
  "typeinfo for fmt::v9::format_error", referenced from:
      fmt::v9::detail::adjust_precision(int&, int) in main.o
      void fmt::v9::detail::vprintf<char, fmt::v9::basic_printf_context<fmt::v9::appender, char>>(fmt::v9::detail::buffer<char>&, fmt::v9::basic_string_view<char>, fmt::v9::basic_format_args<fmt::v9::basic_printf_context<fmt::v9::appender, char>>) in main.o
      int fmt::v9::detail::parse_header<char, void fmt::v9::detail::vprintf<char, fmt::v9::basic_printf_context<fmt::v9::appender, char>>(fmt::v9::detail::buffer<char>&, fmt::v9::basic_string_view<char>, fmt::v9::basic_format_args<fmt::v9::basic_printf_context<fmt::v9::appender, char>>)::'lambda'(int)>(char const*&, char const*, fmt::v9::basic_format_specs<char>&, fmt::v9::basic_printf_context<fmt::v9::appender, char>) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<int, 0>(int) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<unsigned int, 0>(unsigned int) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<long long, 0>(long long) in main.o
      unsigned int fmt::v9::detail::printf_width_handler<char>::operator()<unsigned long long, 0>(unsigned long long) in main.o
      ...
  "vtable for fmt::v9::format_error", referenced from:
      fmt::v9::format_error::format_error(char const*) in main.o
  NOTE: a missing vtable usually means the first non-inline virtual member function has no definition.
ld: symbol(s) not found for architecture x86_64
clang: error: linker command failed with exit code 1 (use -v to see invocation)
make: *** [main] Error 1
```
- fmt 对应的代码是，这里的析构是有default实现的
``` cpp
#if !FMT_MSC_VERSION
FMT_API FMT_FUNC format_error::~format_error() noexcept = default;
#endif
```
- 后面尝试了用 g++ / clang++ 命令行直接编译没有问题
``` bash
clang++ -g -o main src/main.cpp -I./include -I./3rd --std=c++17
```
- 放到xcode编译也没有问题
- 网上查了些资料: -std=libc++, -lstdc++ 但是都不行
- 用 spdlog 的例子编译了下是没有问题的
``` cpp
#include "spdlog/spdlog.h"

int main()
{
    spdlog::info("Welcome to spdlog!");
    spdlog::error("Some error message with arg: {}", 1);

    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:<30}", "left aligned");

    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::debug("This message should be displayed..");

    // change log pattern
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

    // Compile time log levels
    // define SPDLOG_ACTIVE_LEVEL to desired level
    SPDLOG_TRACE("Some trace message with param {}", 42);
    SPDLOG_DEBUG("Some debug message");
}
```
- 然后才发现我的头文件引入顺序上，c++相关的是放在前面的
- 尝试调整了头文件的引入顺序就可以正常编译了
``` cpp
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/bundled/printf.h>
...

#include <sstream>
#include <memory>
#include <utility>
#include <filesystem>
```
- 怀疑是 spdlog/fmt 有一些环境判定的宏相关的代码，但是在 .o -> bin 的时候 .o 没有正确的包含，比如上面的 ~format_error() 的实现函数
- 只是还没有完全弄清楚
