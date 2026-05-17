#pragma once

#include "../DmgrLib.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstddef>
#include <iostream>
#include <string>

class TermSize {
private:
    struct term_measures {
        size_t width{};
        size_t height{};
    };

    term_measures measures{};

    void update() {
        struct winsize w_size{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w_size) == -1) {
            // fallback if ioctl fails
            measures.width  = 80;
            measures.height = 24;
            return;
        }

        measures.width  = w_size.ws_col; // columns = width
        measures.height = w_size.ws_row; // rows    = height
    }


    static std::string stripANSI(const std::string& s) {
        std::string out;
        out.reserve(s.size());

        bool in_escape = false;
        for (char c : s) {
            if (c == '\033') { in_escape = true; continue; }
            if (in_escape) {
                if (c == 'm') in_escape = false;
                continue;
            }
            out.push_back(c);
        }
        return out;
    }

public:
    TermSize() {
        update();
    }

    void refresh() {
        update();
    }

    size_t width() const {
        return measures.width;
    }

    size_t height() const {
        return measures.height;
    }

    void printBoxHeader(const std::string& title = "Info", size_t max_width = 120) {
        refresh();

        size_t box_width = std::min(measures.width, max_width);
        if (box_width < 4) box_width = 4;

        size_t inner = box_width - 2;

        // Title with ANSI formatting
        std::string t = " " + std::string(BOLD) + title + std::string(RESET) + " ";

        // Visible length (strip ANSI)
        size_t t_len = stripANSI(t).size();
        if (t_len > inner) t_len = inner;

        size_t remaining = inner - t_len;
        size_t left  = remaining / 2;
        size_t right = remaining - left;

        std::string left_line, right_line;
        left_line.reserve(left * 3);
        right_line.reserve(right * 3);

        for (size_t i = 0; i < left;  ++i) left_line  += "─";
        for (size_t i = 0; i < right; ++i) right_line += "─";

        std::cout << "┌" << left_line << t << right_line << "┐\n";
    }

    // │ text... │
    void printc(const std::string& s, size_t max_width = 120) {
        refresh();

        size_t box_width = std::min(measures.width, max_width);
        if (box_width < 2) box_width = 2;

        size_t inner = box_width - 2;

        std::string text = " " + s + " ";
        if (text.size() > inner)
            text = text.substr(0, inner);

        size_t padding = inner - text.size();

        std::cout << "│" << text << std::string(padding, ' ') << "│\n";
    }

    // └──────────────┘
    void printBoxFooter(size_t max_width = 120) {
        refresh();

        size_t box_width = std::min(measures.width, max_width);
        if (box_width < 2) box_width = 2;

        size_t inner = box_width - 2;

        std::string line;
        line.reserve(inner * 3);
        for (size_t i = 0; i < inner; ++i)
            line += "─";

        std::cout << "└" << line << "┘\n";
    }
};
