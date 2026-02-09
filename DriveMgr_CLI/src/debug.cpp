#include "../include/debug.h"
#include <iostream>

int debug_msg(const std::string& message, bool is_active) {
    if (!is_active) {
        return 0;
    }

    std::cout << "[DEBUG] " << message << "\n";
    return 1;
}