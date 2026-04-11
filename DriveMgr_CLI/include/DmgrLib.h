/* 
 * DriveMgr - Linux Drive Management Utility
 * Copyright (C) 2025 Dogwalker-kryt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DRIVEFUNCTIONS_H
#define DRIVEFUNCTIONS_H

// C++ libs
#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <array>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>
#include <limits.h>
#include <map>
#include <unordered_map>
#include <optional>
#include <exception>
#include <algorithm>
#include <random>

// Project headers
#include "debug.h"
#include "command_exec.h"
#include "globals.h"
#include "EnvSys.hpp"




// === altTerminal Screen ===
/**
 * @brief Enters into a Alternate Terminal Screen
 */
#define NEWTERMINALSCREEN "\033[?1049h"

/**
 * @brief Leaves the Alternate Terminal Screen
 */
#define LEAVETERMINALSCREEN "\033[?1049l"


// ==================== Color ====================
namespace Color {
    inline std::string reset()   { return "\033[0m"; }
    inline std::string red()     { return g_no_color ? std::string() : "\033[31m"; }
    inline std::string green()   { return g_no_color ? std::string() : "\033[32m"; }
    inline std::string yellow()  { return g_no_color ? std::string() : "\033[33m"; }
    inline std::string blue()    { return g_no_color ? std::string() : "\033[34m"; }
    inline std::string magenta() { return g_no_color ? std::string() : "\033[35m"; }
    inline std::string cyan()    { return g_no_color ? std::string() : "\033[36m"; }
    inline std::string bold()    { return g_no_color ? std::string() : "\033[1m"; }
    inline std::string inverse() { return "\033[7m"; }
}

// Color shortcuts
#define RESET   Color::reset()
#define RED     Color::red()
#define GREEN   Color::green()
#define YELLOW  Color::yellow()
#define BLUE    Color::blue()
#define MAGENTA Color::magenta()
#define CYAN    Color::cyan()
#define BOLD    Color::bold()
#define INVERSE Color::inverse()

const std::unordered_map<std::string, std::string> available_colores {
    {"RED", RED},
    {"GREEN", GREEN},
    {"YELLOW", YELLOW},
    {"BLUE", BLUE},
    {"MAGENTA", MAGENTA},
    {"CYAN", CYAN},
    {"BOLD", BOLD},
    {"INVERSE", INVERSE},
    {"RESET", RESET},
};


// ==================== TerminalAltScreen ====================
class TerminosIO {
    private:
        struct termios oldt, newt;

    public:
        void initiateTerminosInput() {
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
        }

        void enableRawMode() {
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        }

        void restoreTerminal() {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }

        void enableTerminosInput_diableAltTerminal() {
            initiateTerminosInput();
            std::cout << LEAVETERMINALSCREEN;
        }
};

inline TerminosIO term;

// ==================== Logging ====================

/** 
 * @brief On/Off switch var for loggin
 * @param g_no_log will turn to true if you start the programm with the --no-log flag
*/
extern bool g_no_log;

/**
 * @brief Logger class for DriveMgr
 * Provides static methods for logging events with different log levels (error, warning, info, success, dry-run, exec) to a log file.
 * Logs are written to ~/.local/share/DriveMgr/data/log.dat with timestamps and log level tags. The logger respects the g_no_log flag to enable/disable logging and ensures proper file ownership
 */
class Logger {
private:
    /**
     * @brief Log types enumeration
     */
    enum class LogType {
        ERROR,
        WARNING,
        INFO,
        SUCCESS,
        DRYRUN,
        EXEC
    };

    /**
     * @brief map the log types from LogTypes to string
     * Used internally to prepend log messages with a consistent log level tag (e.g., [ERROR], [INFO]) when writing to the log file.
     */
    static inline const char* logMessage(LogType log_type) {
        switch (log_type) {
            case LogType::ERROR: return "[ERROR] ";
            case LogType::WARNING: return "[WARNING] ";
            case LogType::INFO: return "[INFO] ";
            case LogType::SUCCESS: return "[SUCCESS] ";
            case LogType::DRYRUN: return "[DRY-RUN] ";
            case LogType::EXEC: return "[EXEC] ";
            default: return "[UNKNOWN] ";
        }
    }

    /**
     * @brief Log an operation with timestamp to the DriveMgr log file
     * @param operation Description of the operation/event to log
     * 
     * Logs to ~/.local/share/DriveMgr/data/log.dat with format: [DD-MM-YYYY HH:MM] event: <operation>
     * Creates log directory if it doesn't exist. Respects SUDO_USER for proper file ownership.
     */
    static void log(LogType type, const std::string& operation, const bool g_no_log, const char* func) {
        if (g_no_log == false) {

            auto now = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
            char timeStr[100];

            std::strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M", std::localtime(&currentTime));

            std::string log_msg = "[" + std::string(timeStr) + "] event: " + logMessage(type) + operation + " (location: " + std::string(func) + ")";

            std::ofstream log_file(log_path, std::ios::app);

            if (log_file) {

                log_file << log_msg << std::endl;

            } else {
                std::cerr << RED << "[Logger Error] Unable to open log file: " << log_path << " Reason: " << strerror(errno) << RESET <<"\n";
            }

        } else {
            return;
        }
    }
public:

    /**
     * @brief Logs and Error
     * @param msg the Message to be logged in the log file
     * @param g_no_log use the global g_no_log variable for this
     */
    static void error(const std::string &msg, bool g_no_log, const char* func) {
        log(LogType::ERROR, msg, g_no_log, func);
    }

    /**
     * @brief Logs an Warning
     * @param msg the Message to be logged in the log file
     * @param g_no_log use the global g_no_log variable for this
     */
    static void warning(const std::string &msg, bool g_no_log, const char* func) {
        log(LogType::WARNING, msg, g_no_log, func);
    }

    /**
     * @brief Logs an  Info
     * @param msg the Message to be logged in the log file
     * @param g_no_log use the global g_no_log variable for this
     */
    static void info(const std::string &msg, bool g_no_log, const char* func) {
        log(LogType::INFO, msg, g_no_log, func);
    }

    /**
     * @brief Logs an Successs
     * @param msg the Message to be logged in the log file
     * @param g_no_log use the global g_no_log variable for this
     */    
    static void success(const std::string &msg, bool g_no_log, const char* func) {
        log(LogType::SUCCESS, msg, g_no_log, func);
    }

    /**
     * @brief Logs an dry run
     * @param msg the Message to be logged in the log file
     * @param g_no_log use the global g_no_log variable for this
     */ 
    static void dry_run(const std::string &msg, bool g_no_log, const char* func) {
        log(LogType::DRYRUN, msg, g_no_log, func);
    }

    /**
     * @brief Logs an exec
     * @param msg the Message to be logged in the log file
     * @param g_no_log use the global g_no_log variable for this
     */ 
    static void exec(const std::string &msg, bool g_no_log, const char* func) {
        log(LogType::EXEC, msg, g_no_log, func);
    }

    static void clearLoggs(const std::string& path) {
        std::ifstream in(path);
        if (!in) return;

        std::vector<std::string> keep;

        std::string line;

        while (std::getline(in, line)) {

            if (!line.empty() && line[0] == '/') {

                keep.push_back(line);

            }

        }
        
        in.close();

        std::ofstream out(path, std::ofstream::trunc);

        for (const auto& l : keep) {

            out << l << "\n";

        }
    }
};

/**
 * @brief Loggs an error
 * @param msg Message to be logged
 * @param g_no_log use g_no_log for this
 */
#define LOG_ERROR(msg, g_no_log)    Logger::error(msg, g_no_log, __func__)

/**
 * @brief Loggs an warning
 * @param msg Message to be logged
 * @param g_no_log use g_no_log for this
 */
#define LOG_WARNING(msg, g_no_log)  Logger::warning(msg, g_no_log, __func__)

/**
 * @brief Loggs an Info
 * @param msg Message to be logged
 * @param g_no_log use g_no_log for this
 */
#define LOG_INFO(msg, g_no_log)     Logger::info(msg, g_no_log, __func__)

/**
 * @brief Loggs an Success
 * @param msg Message to be logged
 * @param g_no_log use g_no_log for this
 */
#define LOG_SUCCESS(msg, g_no_log)  Logger::success(msg, g_no_log, __func__)

/**
 * @brief Loggs an Dry run
 * @param msg Message to be logged
 * @param g_no_log use g_no_log for this
 */
#define LOG_DRYRUN(msg, g_no_log)   Logger::dry_run(msg, g_no_log, __func__)

/**
 * @brief Loggs an cmd execution
 * @param msg Message to be logged
 * @param g_no_log use g_no_log for this
 */
#define LOG_EXEC(msg, g_no_log)     Logger::exec(msg, g_no_log, __func__)

// ==================== Runtime error and TUI components ====================

/**
 * @brief VERY IMPORTANT FOR TUI TO FUNCTION (DONT DELETE)
 */
extern struct termios oldt, newt;

/**
 * @brief A helper function to throw a std::runtime_error with a given error message.
 *        This function also resets the terminal settings to ensure that the terminal is in a consistent state before throwing the error.
 * @param error_message The error message to include in the std::runtime_error exception.
 * @note This function should be used whenever a runtime error needs to be thrown, as it ensures that the terminal settings are properly reset before throwing the exception.
 */
inline void ldm_runtime_error(const std::string& error_message) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\033[?1049l";
    throw std::runtime_error("[ERROR] " + error_message);
}


// ==================== Signatures for Recovery ====================

/**
 * @struct file_signature
 * @brief File type identification based on magic bytes (file headers)
 * 
 * Used for file carving and recovery - identifies file types by their header bytes
 * without relying on file extensions or filesystem metadata.
 */
struct file_signature {
    std::string extension;              ///< File extension/type name
    std::vector<uint8_t> header;        ///< Magic bytes/header signature for identification
};

/**
 * @brief this is the map for most common filesignatures via hex
 */
static std::map<std::string, file_signature> signatures ={
    /// File type signatures - used for carving deleted files from disk
    /// Maps file type names to their identifying magic bytes
    {"png",  {"png",  {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}}},
    {"jpg",  {"jpg",  {0xFF, 0xD8, 0xFF}}},
    {"elf",  {"elf",  {0x7F, 0x45, 0x4C, 0x46}}},
    {"zip",  {"zip",  {0x50, 0x4B, 0x03, 0x04}}},
    {"pdf",  {"pdf",  {0x25, 0x50, 0x44, 0x46, 0x2D}}},
    {"mp3",  {"mp3",  {0x49, 0x44, 0x33}}},
    {"mp4",  {"mp4",  {0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70}}},
    {"wav",  {"wav",  {0x52, 0x49, 0x46, 0x46}}},
    {"avi",  {"avi",  {0x52, 0x49, 0x46, 0x46}}},
    {"tar.gz", {"tar.gz", {0x1F, 0x8B, 0x08}}},
    {"conf", {"conf", {0x23, 0x21, 0x2F, 0x62, 0x69, 0x6E, 0x2F}}},
    {"txt",  {"txt",  {0x54, 0x45, 0x58, 0x54}}},
    {"sh",   {"sh",   {0x23, 0x21, 0x2F, 0x62, 0x69, 0x6E, 0x2F}}},
    {"xml",  {"xml",  {0x3C, 0x3F, 0x78, 0x6D, 0x6C}}},
    {"html", {"html", {0x3C, 0x21, 0x44, 0x4F, 0x43, 0x54, 0x59, 0x50, 0x45}}},
    {"csv",  {"csv",  {0x49, 0x44, 0x33}}} //often ID3 if they contain metadata
};

// ========= input validation =========

namespace InputValidation {

    /**
     * @brief Validates user input as an integer within a specified range.
     *
     * Reads a full line from standard input and attempts to convert it into an
     * integer. The value must fall within the given range. Error and log messages
     * are handled internally, so callers only need to check the return value.
     *
     * @param min_value The minimum acceptable integer value (inclusive).
     * @param max_value The maximum acceptable integer value (inclusive).
     *
     * @return std::optional<int>
     *         Contains the parsed integer on success, or std::nullopt if the input
     *         is empty, non-numeric, or out of range.
     */
    inline std::optional<int> getInt(const std::vector<int> &valid_ints = {}) {
        std::string s_input;
        std::getline(std::cin, s_input);

        if (s_input.empty()) {

            ERR(ErrorCode::InvalidInput, "int Input expected");
            LOG_ERROR("invalid input; input is empty", g_no_log);
            return std::nullopt;

        }

        try {

            int i_input = std::stoi(s_input);

            if (!valid_ints.empty() && std::find(valid_ints.begin(), valid_ints.end(), i_input) == valid_ints.end()) {

                ERR(ErrorCode::InvalidInput, "Input not in allowed integer list");
                LOG_ERROR("Input not in allowed integer list -> validateIntInput", g_no_log);
                return std::nullopt;

            }

            return i_input;

        } catch (const std::exception& e) {

            ERR(ErrorCode::InvalidInput, "Conversion from string to int failed");
            LOG_ERROR("Conversion from string to int failed -> validateIntInput", g_no_log);
            return std::nullopt;

        }
    }

    /**
     * @ingroup InputValidation
     * @brief Validates user input as an integer within a continuous numeric range.
     *
     * This overload is a convenience wrapper for cases where the allowed integers
     * form a continuous range (e.g., 1–20). It automatically generates a vector
     * containing all integers from @p min_value to @p max_value (inclusive) and
     * forwards the validation to the list-based validateIntInput() function.
     *
     * @param min_value The lowest allowed integer.
     * @param max_value The highest allowed integer.
     *
     * @return std::optional<int>
     *         - Contains the parsed integer if it lies within the range.
     *         - std::nullopt otherwise.
     */
    inline std::optional<int> getInt(int min_value, int max_value) {
        std::vector<int> valid_ints;
        valid_ints.reserve(max_value - min_value + 1);

        for (int i = min_value; i <= max_value; ++i)
            valid_ints.push_back(i);

        return getInt(valid_ints);
    }


    /**
     * @brief Validates user input as a single character.
     *
     * Reads a full line from standard input and checks whether it contains exactly
     * one character. Optionally verifies that the character is part of a list of
     * allowed characters. Error and log messages are handled internally.
     *
     * @param valid_chars Optional list of allowed characters. If empty, any single
     *        character is accepted.
     *
     * @return std::optional<char>
     *         - Contains the parsed character on success.
     *         - std::nullopt if the input is empty, longer than one character,
     *           or not part of the allowed character list.
     */
    inline std::optional<char> getChar(const std::vector<char> &valid_chars = {}) {
        std::string s_input;
        std::getline(std::cin, s_input);

        if (s_input.empty() || s_input.size() != 1) {

            ERR(ErrorCode::InvalidInput, "Input cannot be emtpy or more then 1 char");
            LOG_ERROR("Invalid input; input is empty or more then 1 cahr -> validateCharInput", g_no_log);
            return std::nullopt;

        }

        char c_input = s_input[0];

        if (!valid_chars.empty() && std::find(valid_chars.begin(), valid_chars.end(), c_input) == valid_chars.end()) {

            ERR(ErrorCode::InvalidInput, "Character not allowed");
            LOG_ERROR("Char not in allowed list", g_no_log);
            return std::nullopt;

        }

        return c_input;
    }
}


// ==================== Side/Helper Functions ====================


/**
 * @brief Generates a random 10-character confirmation key consisting of uppercase letters, lowercase letters, and digits.
 * @return A randomly generated confirmation key as a string.
 */
inline std::string confirmationKeyGenerator() {
    std::array<char, 62> chars_for_key = {
        'a','b','c','d','e','f','g','h','i','j',
        'k','l','m','n','o','p','q','r','s','t',
        'u','v','w','x','y','z',
        'A','B','C','D','E','F','G','H','I','J',
        'K','L','M','N','O','P','Q','R','S','T',
        'U','V','W','X','Y','Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };

    static thread_local std::mt19937 gen(std::random_device{}());

    std::uniform_int_distribution<> dist(0, chars_for_key.size() - 1);

    std::string generated_key;

    generated_key.reserve(10);

    for (int i = 0; i < 10; i++) {
        generated_key += chars_for_key[dist(gen)];
    }

    return generated_key;
}

/**
 * @brief Prompts the user for a yes/no confirmation with a custom message.
 * @param prompt The message to display to the user when asking for confirmation.
 */
inline bool askForConfirmation(const std::string &prompt) {
    std::cout << prompt << "(y/n)\n";
    char confirm;
    std::cin >> confirm;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear inputbuffer

    if (confirm != 'Y' && confirm != 'y') {
        std::cout << BOLD << "[INFO] Operation cancelled\n" << RESET;
        LOG_INFO("Operation cancelled", g_no_color);
        return false;
    } 

    return true;
}

/**
 * @brief Handles file paths by prepending the user's home directory to a given relative path.
 * @param file_path The relative file path to be handled in home dir (e.g., "/.config/myapp/config.dat").
 * @returns the ready to use file path with
 */
inline std::string filePathHandler(const std::string &file_path) {
    const char* sudo_user = getenv("SUDO_USER");
    const char* user_env = getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;

    if (!username) {
        std::cerr << RED << "[ERROR] Could not determine username.\n" << RESET;
        LOG_ERROR("Could not determine username", g_no_log);
        return "";
    }

    const struct passwd* pw = getpwnam(username);

    if (!pw) {
        std::cerr << RED << "[ERROR] Could not get home directory for user: " << username << RESET << "\n";
        LOG_ERROR("Failed to get home directory for user: " + std::string(username), g_no_log);
        return "";
    }

    std::string homeDir = pw->pw_dir;
    std::string path = homeDir + file_path;
    
    return path;
}

/**
 * @brief Removes the first n lines from a given string of text.
 * @param text The input string from which to remove lines.
 * @param n The number of lines to remove from the beginning of the text (default is 1).
 * @return A new string with the first n lines removed. If the text has fewer than n lines, returns an empty string.
 */
inline std::string removeFirstLines(const std::string& text, int n = 1) {
    std::string out = text;
    for (int i = 0; i < n; i++) {
        size_t pos = out.find('\n');

        if (pos == std::string::npos)
            return out; // nothing left to remove

        out.erase(0, pos + 1);
    }
    return out;
}

/**
 * @brief This is the End question that is promted when a function failed/finished
 * @param running There is a variable in main that is named 'running', use it for only this
 */
inline void menuQues(bool& running) {   
    std::cout << BOLD <<"\nPress '1' for returning to the main menu, '2' to exit:\n" << RESET;

    auto menuques = InputValidation::getInt({1, 2});

    if (!menuques.has_value()) return;

    if (menuques == 1) {

        running = true;

    } else if (menuques == 2) {

        running = false;
    }
}

inline bool isRoot() {
    return (getuid() == 0);
}

inline bool checkRoot() {
    if (!isRoot()) {
        ERR(ErrorCode::PermissionDenied, "This function requires root privileges. Please run with 'sudo'");
        LOG_ERROR("Attempted to run without root privileges", g_no_log);
        return false;
    }
    return true;
}

inline bool checkRootMetadata() {
    if (!isRoot()) {
        std::cerr << YELLOW << "[WARNING] Running without root may limit functionality. For full access, please run with 'sudo'.\n" << RESET;
        LOG_WARNING("Running without root privileges", g_no_log);
        return false;
    }
    return true;
}

/**
 * Helper function to check if a file exists at the given path.
 * @param path The file path to check.
 * @return true if the file exists, false otherwise.
 */
static bool fileExists(const std::string& path) { struct stat buffer; return (stat(path.c_str(), &buffer) == 0); }



#endif 
