#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "DmgrLib.h"
#include "exec_cmd.h"


enum MenuOptionsMain {
    EXITPROGRAM = 0,        LISTDRIVES = 1,         FORMATDRIVE = 2,        ENCRYPTDECRYPTDRIVE = 3,    RESIZEDRIVE = 4, 
    CHECKDRIVEHEALTH = 5,   ANALYZEDISKSPACE = 6,   OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8,           VIEWINFO = 9,
    MOUNTUNMOUNT = 10,      FORENSIC = 11,          LOGVIEW = 12,           CLONEDRIVE = 13,            CONFIG = 14,          
    FINGERPRINT = 15,       UPDATER = 16,           TESTS = 17
};

class MainMenuIO {
    public:
        /**
         * @brief Its the menu Tui selection with colors
         * @param menuItems its defined in the main functions, it contains all avilable menu items
         */
        static int colorTuiMenu(const std::vector<std::pair<MenuOptionsMain, std::string>> &menuItems) {
            term.enableRawMode();

            int selected = 0;
            int total = (int)menuItems.size();

            auto res = EXEC("clear"); 
            std::cout << "Use Up/Down arrows and Enter to select an option.\n\n";
            std::cout << g_THEME_COLOR << "┌─────────────────────────────────────────────────┐\n" << RESET;
            std::cout << g_THEME_COLOR << "│" << RESET << BOLD << "              DRIVE MANAGEMENT UTILITY           " << RESET << g_THEME_COLOR << "│\n" << RESET;
            std::cout << g_THEME_COLOR << "├─────────────────────────────────────────────────┤\n" << RESET;
            for (size_t i = 0; i < menuItems.size(); ++i) {

                std::cout << g_THEME_COLOR << "│ " << RESET;

                // Build inner content with fixed width
                std::ostringstream inner;
                inner << std::setw(2) << menuItems[i].first << ". " << std::left << std::setw(43) << menuItems[i].second;
                std::string innerStr = inner.str();

                if (menuItems[i].first == 0) {
                    innerStr = g_THEME_COLOR + innerStr + RESET;
                }

                // Print right border and newline
                std::cout << g_THEME_COLOR << " │\n" << RESET;

            }
            std::cout  << g_THEME_COLOR << "└─────────────────────────────────────────────────┘\n" << RESET;

            std::cout << "\033[" << (total + 1) << "A";

            while (true) {

                for (int i = 0; i < total; i++) {
                    std::cout << "\r"; 

                    // Build inner content
                    std::ostringstream inner;
                    inner << std::setw(2) << menuItems[i].first << ". "
                        << std::left << std::setw(43) << menuItems[i].second;

                    std::string innerStr = inner.str();

                    if (menuItems[i].first == 0) {
                        innerStr = g_THEME_COLOR + innerStr + RESET;
                    }

                    std::cout << g_THEME_COLOR << "│ " << RESET;

                    // Apply inverse highlight if selected
                    if (i == selected) std::cout << INVERSE;
                    std::cout << innerStr;
                    if (i == selected) std::cout << RESET;

                    std::cout << g_THEME_COLOR << " │" << RESET << "\n";
                }

                // Move cursor back up to top of menu
                std::cout << "\033[" << total << "A";

                char c;
                if (read(STDIN_FILENO, &c, 1) <= 0) continue;

                if (c == '\x1b') {
                    char seq[2];
                    if (read(STDIN_FILENO, &seq, 2) == 2) {
                        if (seq[1] == 'A') selected = (selected - 1 + total) % total;
                        if (seq[1] == 'B') selected = (selected + 1) % total;
                    }
                }
                else if (c == '\n' || c == '\r') {
                    break;
                }
            }

            // Move cursor down past menu
            std::cout << "\033[" << (total + 1) << "B\n";

            term.restoreTerminal();
            return selected;
        }

        /**
         * @brief Same shit as colorTuiMenu, but with no colors and ">" cursor
         * @param menuItems its defined in the main functions, it contains all avilable menu items
         */
        static int noColorTuiMenu(const std::vector<std::pair<MenuOptionsMain, std::string>> &menuItems) {
            term.enableRawMode();

            int selected = 0;
            int total = (int)menuItems.size();

            auto res = EXEC("clear");
            std::cout << "Use Up/Down arrows and Enter to select an option.\n\n";
            std::cout << "┌─────────────────────────────────────────────────────┐\n";
            std::cout << "│" << BOLD << "                DRIVE MANAGEMENT UTILITY             " << RESET << "│\n";
            std::cout << "├─────────────────────────────────────────────────────┤\n";

            for (size_t i = 0; i < menuItems.size(); ++i) {

                std::cout << "│ ";

                // Build inner content with fixed width
                std::ostringstream inner;
                inner << std::setw(2) << menuItems[i].first << ". "
                    << std::left << std::setw(44) << menuItems[i].second;

                std::cout << inner.str();

                std::cout << "  │\n";
            }

            std::cout << "└─────────────────────────────────────────────────────┘\n";

            // Move cursor UP to where the first selectable line is
            std::cout << "\033[" << (total + 1) << "A";

            while (true) {
                // Redraw selector arrows
                for (int i = 0; i < total; i++) {
                    std::cout << "\r"; // go to start of line

                    if (i == selected) std::cout << "│ > ";
                    else std::cout << "│   ";

                    std::ostringstream inner;
                    inner << std::setw(2) << menuItems[i].first << ". "
                          << std::left << std::setw(44) << menuItems[i].second;

                    std::cout << inner.str() << "  │\n";
                }

                // Move cursor back up to top of menu
                std::cout << "\033[" << total << "A";

                char c;
                if (read(STDIN_FILENO, &c, 1) <= 0) continue;

                if (c == '\x1b') {
                    char seq[2];
                    if (read(STDIN_FILENO, &seq, 2) == 2) {
                        if (seq[1] == 'A') selected = (selected - 1 + total) % total; // up
                        if (seq[1] == 'B') selected = (selected + 1) % total;         // down
                    }
                } 
                else if (c == '\n' || c == '\r') {
                    break;
                }
            }

            // Move cursor down past menu
            std::cout << "\033[" << (total + 1) << "B\n";

            term.restoreTerminal();
            return selected;
        }
};

class GenericMenuIO {
private:
    /**
     * @brief Helper to print a single menu line with proper padding and selection highlight
     * @param selected whether this line is currently selected
     * @param item the menu item to print
     * @param inner_width the width of the content area for padding calculations
     */
    static void printMenuLine(bool selected, const std::pair<int,std::string>& item, size_t inner_width) {
        std::string num = std::to_string(item.first) + ". ";
        size_t item_len = num.length() + item.second.length();
        size_t padding  = (inner_width > item_len) ? (inner_width - item_len) : 0;

        std::cout << "│ " << (selected ? "> " : "  ")
                  << num
                  << item.second
                  << std::string(padding, ' ')
                  << " │\n";
    }

    /**
     * @brief Compute the inner width needed to fit the title and all menu items, used for consistent box sizing
     * @param title the menu title
     * @param items the menu items to consider for width calculations
     */
    static size_t computeInnerWidth(const std::string& title, const std::vector<std::pair<int,std::string>>& items) {
        size_t longest = title.length();

        for (const auto& item : items) {

            size_t len = std::to_string(item.first).length() + 2 + item.second.length();
            longest = std::max(longest, len);

        }

        return longest;
    }

    static void printDash(size_t n) {
        while (n--) std::cout << "─";
    }

public:
    /**
     * @brief A more flexible TUI menu that can be used for any menu in the program, not just the main menu. It uses box-drawing characters and supports dynamic width based on content.
     * @param title the title to display at the top of the menu
     * @param menuItems the list of menu items to display, each with an integer ID and a string label
     */
    static int noColorTuiMenu(const std::string &title, const std::vector<std::pair<int, std::string>> &menuItems) {
        term.enableRawMode();

        int selected = 0;
        int total = menuItems.size();

        // Width calculations
        size_t inner_width = computeInnerWidth(title, menuItems);
        size_t box_width   = inner_width + 4;

        // Title with padding
        std::string title_pad = " " + title + " ";
        size_t title_len = title_pad.length();

        size_t dash_total = box_width - title_len;
        size_t dash_left  = dash_total / 2;
        size_t dash_right = dash_total - dash_left;

        // Title bar
        std::cout << "\n┌";
        printDash(dash_left);
        std::cout << BOLD << title_pad << RESET;
        printDash(dash_right);
        std::cout << "┐\n";

        // Static items
        for (const auto& item : menuItems) {

            printMenuLine(false, item, inner_width);

        }

        // Bottom border
        std::cout << "└";
        printDash(box_width);
        std::cout << "┘\n";

        // Move cursor up to first item
        std::cout << "\r\033[" << (total + 1) << "A";

        // Selection loop
        while (true) {

            for (int i = 0; i < total; i++) {

                std::cout << "\r";
                printMenuLine(i == selected, menuItems[i], inner_width);

            }

            std::cout << "\033[" << total << "A";

            char c;
            if (read(STDIN_FILENO, &c, 1) <= 0) continue;

            if (c == '\x1b') {

                char seq[2];

                if (read(STDIN_FILENO, &seq, 2) == 2) {

                    if (seq[1] == 'A') selected = (selected - 1 + total) % total;
                    if (seq[1] == 'B') selected = (selected + 1) % total;

                }

            }
            else if (c == '\n' || c == '\r') {
                break;
            }
        }

        // Move cursor below menu
        std::cout << "\r\033[" << (total + 1) << "B\n";

        term.restoreTerminal();
        return menuItems[selected].first;
    }
};
