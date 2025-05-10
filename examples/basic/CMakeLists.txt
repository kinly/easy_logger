add_executable(basic_example main.cpp)
add_definitions(-DSPDLOG_USE_STD_FORMAT)
set(CMAKE_CXX_STANDARD 20)
target_include_directories(basic_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/spdlog/include
)
