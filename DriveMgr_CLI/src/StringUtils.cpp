#include "../include/StringUtils.hpp"


std::string StrUtils::checkIfEmpty(const std::string &_str) {
    if (_str.empty()) {

        dmgr_runtime_error("Input string cannot be empty");
        return ""; // This line will never be reached, but is needed to satisfy the compiler
    }

    return _str;
}

std::string StrUtils::trimWhiteSpace(const std::string &_str) {
    std::string str = checkIfEmpty(_str);

    size_t start_of_str = str.find_first_not_of(" \t\n\r\f\v");

    if (start_of_str == std::string::npos) {
        return "";
    }

    size_t end_of_str = str.find_last_not_of(" \t\n\r\f\v");

    return str.substr(start_of_str, end_of_str - start_of_str + 1);
}

std::string StrUtils::toLowerString(const std::string &_str) {
    std::string str = checkIfEmpty(_str);

    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str;
}

std::string StrUtils::toUpperString(const std::string &_str) {
    std::string str = checkIfEmpty(_str);

    std::string upper_str = str;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);
    return upper_str;
}

std::string StrUtils::removeFirstLines(const std::string &_str, int n) {
    std::string v_str = checkIfEmpty(_str);

    for (int i = 0; i < n; i++) {
        size_t pos = v_str.find('\n');

        if (pos == std::string::npos)
            return v_str; 

        v_str.erase(0, pos + 1);
    }
    return v_str;
}

std::string StrUtils::replaceAll(const std::string &_str) {
    std::string str = checkIfEmpty(_str);

    std::string new_str = str;

    return new_str;
}