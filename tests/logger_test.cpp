#include <easy_logger/logger.h>
#include <gtest/gtest.h>

TEST(LoggerTest, BasicLogging) {
    util::logger::easy_logger::init();
    util::logger::easy_logger::get().init("test.log");

    LOG_INFO("Test message");
    STM_INFO<','>(1, 2, 3);
}