#include <easy_logger/logger.h>

int main() {
    util::logger::easy_logger::init();
    util::logger::easy_logger::get().init("example.log");

    LOG_INFO("Basic example");
    STM_INFO<','>(1, 2, 3);

    return 0;
}