# 2025-04-18 更新：
- 抛弃掉之前的版本，使用 logger.h 作为基础版本
- 去掉复杂的内容，只保留日志最简单的样子
- 关键点是增加了编译期的 std::format placeholders
- 目前 logger.h 里保留的是简易的原始版本
- auto_format_rules.h 里是一个期望自定义 format 样式的版本（模板类重载，具体下面的测试用例：test_struct）
- 也许这样才更合适，后期考虑把 filename 提出来作为模版，这样还可以用 spdlog 做些其他事情
- 以下是这次版本 AI 协助产出的文档：

# 编译期格式字符串自动推导工具：实现与演进记录

本文记录了我在实现一个**编译期类型感知的格式字符串生成工具**的过程中所遇到的问题、优化思路与最终实现细节，旨在为有类似需求的 C++ 开发者提供思路。

## 背景需求

在使用 `std::format` 或 `spdlog::log` 等格式化日志时，频繁地手写格式字符串容易出错、不统一，也不够自动化。尤其是当参数较多，且类型不同（如 `float` 需保留小数）时，维护一套规范的格式变得棘手。

为此，我希望实现以下目标：

- ✅ 编译期推导格式字符串，无运行期开销
- ✅ 不依赖 RTTI、反射或宏系统
- ✅ 支持 tuple 嵌套类型展开
- ✅ 针对 float/double 提供默认精度控制（如 `{:.2f}`）
- ✅ 可缓存格式字符串结构，供日志框架使用

---

## 常见实现尝试与问题

### 尝试 1：直接拼接 `{}` 占位符

最早我直接构造 "{} {} {}" 类似的字符串，但无法体现类型特性，float 没法自动变成 `{:.2f}`，而且对 tuple、结构体等复杂类型也不友好。

### 尝试 2：基于格式描述元组生成

我尝试写了如下模板：

```cpp
std::string make_fmt<float, int, double>() => "{:.2f} {} {:.2f}"
```

思路不错，但写法冗长、不能递归 tuple 内容。

### 尝试 3：递归处理 tuple

我引入了 SFINAE 检查是否 tuple，并用 index_sequence 展开内部字段，但发现格式串缓存和拼接过程容易越界或丢失分隔符，遂引入中间格式缓存。

最终演化出了以下结构。

---

## 最终代码实现与解释

### 顶层宏配置

```cpp
#ifndef AUTO_FORMAT_LOGGER_MAX_ARGS
#define AUTO_FORMAT_LOGGER_MAX_ARGS 20
#endif
```

允许你限制支持的最大参数数量，避免浪费空间或越界访问。

### 类型到格式映射机制

```cpp
template <typename tt>
struct type_format { static constexpr const char* value = "{}"; };

// 对于 float/double 提供默认格式
template <> struct type_format<float> { static constexpr const char* value = "{:.2f}"; };
template <> struct type_format<double> { static constexpr const char* value = "{:.2f}"; };
```

这是系统的核心机制，用于指定不同类型在格式字符串中的默认格式。可以很方便扩展其他类型，比如 `bool` 显示为 true/false，或者为用户自定义类型注册格式化方案。

### 检测类型是否为 tuple

```cpp
template <typename tt>
concept is_tuple_like = requires { typename std::tuple_size<tt>::type; };
```

使用 C++20 的 `concept` 特性，判断某个类型是否具备 tuple 接口。

### 编译期格式符收集（支持 tuple 展开）

```cpp
template <typename tt, typename collector_tt>
consteval void collect_type_formats(collector_tt collector);
```

这个递归 constexpr 函数会：
- 如果是 tuple，就展开 tuple 的每个元素，继续递归收集
- 如果是普通类型，就调用 `type_format<T>` 获取对应格式字符串，并送入 collector

### 编译期拼接格式串

```cpp
template <std::size_t count_vv, char sep_vv = ' '>
consteval auto make_format_string(const std::array<const char*, count_vv>& formats, std::size_t used);
```

此函数将格式占位符如 `"{:.2f}"`, "{}" 等拼接为完整格式字符串，如：
```text
{:.2f} {} {:.2f}
```
支持指定分隔符，默认是空格。

注意我们对格式字符串进行了逆序整数输出（用于兜底），并用 `std::reverse` 修正顺序，这属于基本的编译期字符处理技巧。

### 编译期格式缓存结构

```cpp
template <typename... args_tt>
struct type_format_string_placeholders {
  static constexpr auto fmt = make_type_format_string_ct<args_tt...>();
  static constexpr auto sv = std::string_view{fmt.first.data(), fmt.second};
};
```

通过该结构，我们可以以任意模板参数组调用 `type_format_string_placeholders<Ts...>::sv` 获取格式字符串。

由于格式串内容是 `consteval` 拼接的 `std::array<char>`，我们提供 `.sv` 封装为 string_view，供 `std::vformat` 或 `spdlog::log` 使用。

### 多参数处理流程总结：

1. 模板参数包输入类型
2. 展开收集每个类型的格式占位符
3. 编译期拼接为完整字符串 `char[]`
4. 提供静态缓存字段供外部访问

---

## 示例使用

```cpp
constexpr auto fmt = auto_format_rules::detail::make_type_format_string_ct<float, int, float, float>();
std::string_view sv(fmt.first.data(), fmt.second);

float a = 1.2345f, c = 3.14f, d = 2.71f;
int b = 42;
std::string result = std::vformat(sv, std::make_format_args(a, b, c, d));
std::cout << result << '\n';
// 输出: 1.23 42 3.14 2.71
```

支持空参数：
```cpp
static constexpr auto fmt2 = auto_format_rules::detail::type_format_string_placeholders<>::sv;
```

支持 tuple 嵌套：
```cpp
using my_tuple = std::tuple<int, float, std::tuple<double, int>>;
constexpr auto fmt = auto_format_rules::detail::type_format_string_placeholders<my_tuple>::sv;
```

---

## 后续可扩展点

- 支持结构体字段名导出（结合反射或宏）
- 对于特定字段自动附加单位（如 `%`, `ms`, `MB`）
- 自定义格式规则注册机制，如：`register_format<MyType>("{:X}")`
- 添加 format string 校验器（在编译期验证格式与参数是否一致）

---

## 总结

这个工具让我们能够完全在编译期自动生成类型安全、规范统一的格式字符串，非常适合日志系统、调试工具、序列化预设等场景。

如果你也在追求高性能和低维护成本的日志格式生成，不妨尝试将这个方案引入你的项目！

欢迎评论交流和优化建议！


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
