# Easy Logger

一个基于spdlog的C++20日志库封装，提供简单易用的接口和编译期格式字符串生成。

## 特性

- Header-only，无需编译
- 基于C++20和spdlog
- 编译期格式字符串生成
- 支持自定义分隔符
- 支持结构体格式化（通过std::formatter）
- 零运行时开销
- 线程安全
- 支持多种日志级别
- 支持文件和控制台输出

## 依赖

- C++20兼容的编译器
- [spdlog](https://github.com/gabime/spdlog)（作为 submodule）

## 获取与构建

```bash
git clone https://github.com/yourusername/easy_logger.git
cd easy_logger
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make install
```

## Include 路径设置

请确保你的 include path 包含：
- easy_logger 的头文件目录：`easy_logger/include`
- spdlog 的头文件目录：`easy_logger/3rd/spdlog/include`

- spdlog 需要启用的编译选项.
```cmake
add_definitions(-DSPDLOG_USE_STD_FORMAT)
set(CMAKE_CXX_STANDARD 20)
```

例如 g++ 编译参数：
```bash
g++ -Ieasy_logger/include -Ieasy_logger/3rd/spdlog/include your_code.cpp
```

## 使用示例

```cpp
#include <easy_logger/logger.h>

int main() {
    util::logger::easy_logger::get().init("app.log");

    LOG_INFO("Hello, {}!", "World");
    STM_INFO(1, 2, 3);  // 输出: 1,2,3
    return 0;
}
```

## 日志级别

- TRACE
- DEBUG
- INFO
- WARN
- ERROR
- CRITICAL

## 贡献

欢迎提交Issue和Pull Request！

## 许可证

MIT License

