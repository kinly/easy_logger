#include "easy_logger.h"

#include <iostream>

int main() 
{
    util::logger::easy_logger<util::logger::key_default>::get().init(util::logger::options_default);

    PRINT_WARN(util::logger::key_default, "warn log, %d-%d", 1, 2);

    return 0;
}
