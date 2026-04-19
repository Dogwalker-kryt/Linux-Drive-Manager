#pragma once

#include <iostream>
#include <string>
#include "debug.h"
#include <algorithm>
#include <optional>
#include "TermiosIO.h"
#include "dmgr_runtime_error.hpp"

namespace StrUtils {
    /**
     * @brief checks if an string is emtpy
     */
    std::string checkIfEmpty(const std::string &_str);

    /**
     * @brief trims white spaces, new line characters and more
     * @returns trimmed string
     */
    std::string trimWhiteSpace(const std::string &_str);

    /**
     * @brief sets all characters to lowercase
     * @returns lowercase string
     */
    std::string toLowerString(const std::string &_str);

    /**
     * @brief set all characters to uppercase
     * @returns uppercase string
     */
    std::string toUpperString(const std::string &_str);

    /**
     * @brief Removes the first n lines from a given string of text.
     * @param text The input string from which to remove lines.
     * @param n The number of lines to remove from the beginning of the text (default is 1).
     * @return A new string with the first n lines removed. If the text has fewer than n lines, returns an empty string.
     */
    std::string removeFirstLines(const std::string &_str, int n = 1);

    /**
     * @brief replaces the complete string 
     * @returns reaplaces string
     */
    std::string replaceAll(const std::string &_str);

}