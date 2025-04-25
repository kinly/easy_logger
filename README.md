# spdlog wrapper C++20 版本（直接使用std::format）：
- 基于 c++20, [spdlog](https://github.com/gabime/spdlog)
- 抛弃掉之前的版本(`old_version`)，使用 `logger.h` 作为基础版本
- 去掉复杂的内容，只保留日志最简单的样子
- 关键点是增加了编译期的 `std::format placeholders`
- 目前 `logger.h` 里保留的是简易的原始版本
- `auto_format_rules.h` 里是一个期望自定义 `format` 样式的版本（模板类重载，具体下面的测试用例：`test_struct`）
- 也许这样才更合适，后期考虑把 `filename` 提出来作为模版，这样还可以用 `spdlog` 做些其他事情

# c++11 版本：
- 替换 std::format 相关内容即可，可参考 `old_version` 里的方式

------------------------------------------------------------
- (以下是这次版本 AI 协助产出的文档：)
- (以下是这次版本 AI 协助产出的文档：)
- (以下是这次版本 AI 协助产出的文档：)

## 编译期格式字符串自动推导工具：实现与演进记录

本文记录了我在实现一个**编译期类型感知的格式字符串生成工具**的过程中所遇到的问题、优化思路与最终实现细节，旨在为有类似需求的 C++ 开发者提供思路。

### 背景需求

在使用 `std::format` 或 `spdlog::log` 等格式化日志时，频繁地手写格式字符串容易出错、不统一，也不够自动化。尤其是当参数较多，且类型不同（如 `float` 需保留小数）时，维护一套规范的格式变得棘手。

为此，我希望实现以下目标：

- ✅ 编译期推导格式字符串，无运行期开销
- ✅ 不依赖 RTTI、反射或宏系统
- ✅ 支持 tuple 嵌套类型展开
- ✅ 针对 float/double 提供默认精度控制（如 `{:.2f}`）
- ✅ 可缓存格式字符串结构，供日志框架使用

---

### 常见实现尝试与问题

#### 尝试 1：直接拼接 `{}` 占位符

最早我直接构造 "{} {} {}" 类似的字符串，但无法体现类型特性，float 没法自动变成 `{:.2f}`，而且对 tuple、结构体等复杂类型也不友好。

#### 尝试 2：基于格式描述元组生成

我尝试写了如下模板：

```cpp
std::string make_fmt<float, int, double>() => "{:.2f} {} {:.2f}"
```

思路不错，但写法冗长、不能递归 tuple 内容。

#### 尝试 3：递归处理 tuple

我引入了 SFINAE 检查是否 tuple，并用 index_sequence 展开内部字段，但发现格式串缓存和拼接过程容易越界或丢失分隔符，遂引入中间格式缓存。

最终演化出了以下结构。

---

### 最终代码实现与解释

#### 顶层宏配置

```cpp
#ifndef AUTO_FORMAT_LOGGER_MAX_ARGS
#define AUTO_FORMAT_LOGGER_MAX_ARGS 20
#endif
```

允许你限制支持的最大参数数量，避免浪费空间或越界访问。

#### 类型到格式映射机制

```cpp
template <typename tt>
struct type_format { static constexpr const char* value = "{}"; };

// 对于 float/double 提供默认格式
template <> struct type_format<float> { static constexpr const char* value = "{:.2f}"; };
template <> struct type_format<double> { static constexpr const char* value = "{:.2f}"; };
```

这是系统的核心机制，用于指定不同类型在格式字符串中的默认格式。可以很方便扩展其他类型，比如 `bool` 显示为 true/false，或者为用户自定义类型注册格式化方案。

#### 检测类型是否为 tuple

```cpp
template <typename tt>
concept is_tuple_like = requires { typename std::tuple_size<tt>::type; };
```

使用 C++20 的 `concept` 特性，判断某个类型是否具备 tuple 接口。

#### 编译期格式符收集（支持 tuple 展开）

```cpp
template <typename tt, typename collector_tt>
consteval void collect_type_formats(collector_tt collector);
```

这个递归 constexpr 函数会：
- 如果是 tuple，就展开 tuple 的每个元素，继续递归收集
- 如果是普通类型，就调用 `type_format<T>` 获取对应格式字符串，并送入 collector

#### 编译期拼接格式串

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

#### 编译期格式缓存结构

```cpp
template <typename... args_tt>
struct type_format_string_placeholders {
  static constexpr auto fmt = make_type_format_string_ct<args_tt...>();
  static constexpr auto sv = std::string_view{fmt.first.data(), fmt.second};
};
```

通过该结构，我们可以以任意模板参数组调用 `type_format_string_placeholders<Ts...>::sv` 获取格式字符串。

由于格式串内容是 `consteval` 拼接的 `std::array<char>`，我们提供 `.sv` 封装为 string_view，供 `std::vformat` 或 `spdlog::log` 使用。

#### 多参数处理流程总结：

1. 模板参数包输入类型
2. 展开收集每个类型的格式占位符
3. 编译期拼接为完整字符串 `char[]`
4. 提供静态缓存字段供外部访问

---

### 示例使用

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

### 后续可扩展点

- 支持结构体字段名导出（结合反射或宏）
- 对于特定字段自动附加单位（如 `%`, `ms`, `MB`）
- 自定义格式规则注册机制，如：`register_format<MyType>("{:X}")`
- 添加 format string 校验器（在编译期验证格式与参数是否一致）

---

### 总结

这个工具让我们能够完全在编译期自动生成类型安全、规范统一的格式字符串，非常适合日志系统、调试工具、序列化预设等场景。

如果你也在追求高性能和低维护成本的日志格式生成，不妨尝试将这个方案引入你的项目！

欢迎评论交流和优化建议！

