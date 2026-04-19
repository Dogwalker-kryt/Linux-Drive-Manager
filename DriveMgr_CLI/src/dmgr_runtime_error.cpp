#include "../include/dmgr_runtime_error.hpp"

void dmgr_runtime_error(const std::string &msg) {
    TerminosIO::restoreTerminal();
    std::cout << LEAVETERMINALSCREEN;
    throw std::runtime_error("[ERROR] " + msg);
}