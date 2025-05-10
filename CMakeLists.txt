cmake_minimum_required(VERSION 3.14)
project(easy_logger VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(EASY_LOGGER_BUILD_TESTS "Build tests" ON)
option(EASY_LOGGER_BUILD_EXAMPLES "Build examples" ON)

# 添加spdlog依赖
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# 创建接口库
add_library(easy_logger INTERFACE)
target_include_directories(easy_logger INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(easy_logger INTERFACE spdlog::spdlog)

# 安装配置
include(GNUInstallDirs)
install(TARGETS easy_logger
    EXPORT easy_logger-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 导出目标
install(EXPORT easy_logger-targets
    FILE easy_logger-targets.cmake
    NAMESPACE easy_logger::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/easy_logger
)

# 配置包版本文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/easy_logger-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 配置包配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/easy_logger-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/easy_logger-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/easy_logger
)

# 安装配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/easy_logger-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/easy_logger-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/easy_logger
)

# 测试
if(EASY_LOGGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
if(EASY_LOGGER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()