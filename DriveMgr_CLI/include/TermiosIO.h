#pragma once

#include <termios.h>
#include <unistd.h>
#include "globals.h"

/** @brief Class for handling terminal I/O operations */
class TerminosIO {
    private:
        static struct termios oldt, newt;

    public:
        static inline void initiateTerminosInput() {
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
        }

        static inline void enableRawMode() {
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        }

        static inline void restoreTerminal() {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }

        static inline void enableTerminosInput_diableAltTerminal() {
            initiateTerminosInput();
            std::cout << LEAVETERMINALSCREEN;
        }
};

// TerminosIO term;
