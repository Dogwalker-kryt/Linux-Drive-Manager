#pragma once

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
        static int colorTuiMenu(const std::vector<std::pair<MenuOptionsMain, std::string>> &menuItems);

        /**
         * @brief Same shit as colorTuiMenu, but with no colors and ">" cursor
         * @param menuItems its defined in the main functions, it contains all avilable menu items
         */
        static int noColorTuiMenu(const std::vector<std::pair<MenuOptionsMain, std::string>> &menuItems);
};

class GenericMenuIO {
private:
    /**
     * @brief Helper to print a single menu line with proper padding and selection highlight
     * @param selected whether this line is currently selected
     * @param item the menu item to print
     * @param inner_width the width of the content area for padding calculations
     */
    static void printMenuLine(bool selected, const std::pair<int,std::string>& item, size_t inner_width);

    /**
     * @brief Compute the inner width needed to fit the title and all menu items, used for consistent box sizing
     * @param title the menu title
     * @param items the menu items to consider for width calculations
     */
    static size_t computeInnerWidth(const std::string& title, const std::vector<std::pair<int,std::string>>& items);

    static void printDash(size_t n);

public:
    /**
     * @brief A more flexible TUI menu that can be used for any menu in the program, not just the main menu. It uses box-drawing characters and supports dynamic width based on content.
     * @param title the title to display at the top of the menu
     * @param menuItems the list of menu items to display, each with an integer ID and a string label
     */
    static int noColorTuiMenu(const std::string &title, const std::vector<std::pair<int, std::string>> &menuItems);
};