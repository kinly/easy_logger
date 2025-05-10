find_package(GTest REQUIRED)
add_executable(logger_test logger_test.cpp)
target_link_libraries(logger_test PRIVATE easy_logger GTest::GTest GTest::Main)
add_test(NAME logger_test COMMAND logger_test)