cmake_minimum_required(VERSION 3.14)
project(easy_logger VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(EASY_LOGGER_BUILD_TESTS "Build tests" OFF)  # 暂时禁用测试
option(EASY_LOGGER_BUILD_EXAMPLES "Build examples" ON)

# 安装配置
include(GNUInstallDirs)

# 只做头文件安装
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# 示例和测试可选
if(EASY_LOGGER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(EASY_LOGGER_BUILD_TESTS)
    add_subdirectory(tests)
endif()