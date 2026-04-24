#include "../include/Spinner.hpp"


std::array<std::string, 4> SimpleSpinner::frames = {"|", "/", "—", "\\"};
int SimpleSpinner::idx = 0;

void SimpleSpinner::tick() {
    std::cout << "\r" << frames[idx++ % 4] << " Running..." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void SimpleSpinner::done() {
    std::cout << GREEN << "\r✓ " << RESET << "Done       \n" << std::flush;
}

void SimpleSpinner::progressSpinner(std::atomic<bool> &b_done) {
    while (!b_done) {
        
        tick();

    }

    done();
    return;
}