#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <type_traits>

#ifndef AUTO_FORMAT_LOGGER_MAX_ARGS
#define AUTO_FORMAT_LOGGER_MAX_ARGS 20
#endif

namespace auto_format_rules {
namespace detail {

template <typename tt>
struct type_format {
  static constexpr const char* value = "{}";
};

template <>
struct type_format<float> {
  static constexpr const char* value = "{:.2f}";
};

template <>
struct type_format<double> {
  static constexpr const char* value = "{:.2f}";
};

template <typename tt>
concept is_tuple_like = requires { typename std::tuple_size<tt>::type; };

template <typename tt, typename collector_tt>
consteval void collect_type_formats(collector_tt collector) {
  if constexpr (is_tuple_like<tt>) {
    constexpr size_t size = std::tuple_size_v<tt>;
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      (collect_type_formats<std::tuple_element_t<Is, tt>>(collector), ...);
    }(std::make_index_sequence<size>{});
  } else {
    collector(type_format<std::decay_t<tt>>::value);
  }
}

template <std::size_t index_vv = 0, typename tt, typename... args_tt>
consteval void consteval_collect_format_array(
  std::array<const char*, AUTO_FORMAT_LOGGER_MAX_ARGS>& out, std::size_t& index) {
  collect_type_formats<tt>([&](const char* s) { out[index++] = s; });
  if constexpr (sizeof...(args_tt) > 0) {
    consteval_collect_format_array<index_vv + 1, args_tt...>(out, index);
  }
}

template <typename... args_tt>
consteval auto collect_format_array() {
  std::array<const char*, AUTO_FORMAT_LOGGER_MAX_ARGS> out{};
  std::size_t index = 0;

  if constexpr (sizeof...(args_tt) > 0) {
    consteval_collect_format_array<0, args_tt...>(out, index);
  }

  return std::pair{out, index};
}

template <std::size_t count_vv, char sep_vv = ' '>
consteval auto make_format_string(const std::array<const char*, count_vv>& formats, std::size_t used) {
  std::array<char, AUTO_FORMAT_LOGGER_MAX_ARGS * 16> buf{};
  std::size_t pos = 0;
  for (std::size_t i = 0; i < used; ++i) {
    const char* f = formats[i];
    if (f == nullptr || *f == '\0') {
      buf[pos++] = '{';
      std::size_t tmp = i;
      std::size_t begin = pos;
      do {
        buf[pos++] = '0' + tmp % 10;
        tmp /= 10;
      } while (tmp > 0);
      std::reverse(buf.begin() + begin, buf.begin() + pos);
      buf[pos++] = '}';
    } else {
      for (const char* p = f; *p != '\0'; ++p)
        buf[pos++] = *p;
    }

    if (i < used - 1)
      buf[pos++] = sep_vv;
  }

  std::array<char, AUTO_FORMAT_LOGGER_MAX_ARGS * 16> result{};
  std::copy_n(buf.begin(), pos, result.begin());
  return std::pair{result, pos};
}

template <typename... args_tt>
consteval auto make_type_format_string_ct() {
  // if constexpr (sizeof...(args_tt) == 0) {
  //   return std::pair{std::array<char, 1>{""}, 0ull};
  // }
  auto [formats, used] = collect_format_array<args_tt...>();
  return make_format_string(formats, used);
}

template <typename... args_tt>
struct type_format_string_placeholders {
  static constexpr auto fmt = make_type_format_string_ct<args_tt...>();
  static constexpr auto sv = std::string_view{fmt.first.data(), fmt.second};
};

}  // namespace detail
}  // namespace auto_format_rules

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/// test code
namespace auto_format_rules {
namespace detail {
struct test_struct {
  
};

template<>
struct type_format<test_struct> {
  static constexpr const char* value = "struct:{}";
};
}  // namespace detail
}  // namespace auto_format_rules

void test_auto_format_rules() {
  constexpr auto fmt = auto_format_rules::detail::make_type_format_string_ct<float, int, float, float>();
  std::string_view sv(fmt.first.data(), fmt.second);
  float a = 1.2345f, c = 3.14f, d = 2.71f;
  int b = 42;
  std::string result = std::vformat(sv, std::make_format_args(a, b, c, d));
  std::cout << "Formatted: " << result << '\n';

  auto fmt_st = auto_format_rules::detail::make_type_format_string_ct<auto_format_rules::detail::test_struct>();

  static constexpr auto fmt2 = auto_format_rules::detail::type_format_string_placeholders<>::sv;
  static constexpr auto fmt3 = auto_format_rules::detail::type_format_string_placeholders<>::sv;
  std::cout << fmt2.data() << std::endl;
  std::cout << fmt3.data() << std::endl;
}

