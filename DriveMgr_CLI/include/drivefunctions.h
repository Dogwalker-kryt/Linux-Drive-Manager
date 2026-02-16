
#ifndef DRIVEFUNCTIONS_H
#define DRIVEFUNCTIONS_H

#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <array>
#include <limits>
#include <iomanip>
#include <filesystem>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <limits.h>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/select.h>
#include <sys/time.h>
#include "command_exec.h"

extern bool g_no_color;
extern bool g_dry_run;

// Color
namespace Color {
    inline std::string reset()   { return g_no_color ? std::string() : "\033[0m"; }
    inline std::string red()     { return g_no_color ? std::string() : "\033[31m"; }
    inline std::string green()   { return g_no_color ? std::string() : "\033[32m"; }
    inline std::string yellow()  { return g_no_color ? std::string() : "\033[33m"; }
    inline std::string blue()    { return g_no_color ? std::string() : "\033[34m"; }
    inline std::string magenta() { return g_no_color ? std::string() : "\033[35m"; }
    inline std::string cyan()    { return g_no_color ? std::string() : "\033[36m"; }
    inline std::string bold()    { return g_no_color ? std::string() : "\033[1m"; }
    inline std::string inverse() { return g_no_color ? std::string() : "\033[7m"; }
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

/** 
 * @brief On/Off switch var for loggin
 * @param g_no_log will turn to true if you start the programm with the --no-log flag
*/
extern bool g_no_log;

class Logger {
public:
    /**
     * @brief Log an operation with timestamp to the DriveMgr log file
     * @param operation Description of the operation/event to log
     * 
     * Logs to ~/.local/share/DriveMgr/data/log.dat with format: [DD-MM-YYYY HH:MM] event: <operation>
     * Creates log directory if it doesn't exist. Respects SUDO_USER for proper file ownership.
     */
    static void log(const std::string& operation, const bool g_no_log) {
        if (g_no_log == false) {
            auto now = std::chrono::system_clock::now();
            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
            char timeStr[100];
            std::strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M", std::localtime(&currentTime));
            //"%d-%m-%Y %M:%H" // new
            //"%Y-%m-%d %H:%M:%S" // old
            std::string logMsg = "[" + std::string(timeStr) + "] event: " + operation;

            const char* sudo_user = std::getenv("SUDO_USER");
            const char* user_env = std::getenv("USER");
            const char* username = sudo_user ? sudo_user : user_env;
            
            if (!username) {
                std::cerr << "[Logger Error] Could not determine username.\n";
                return;
            }

            struct passwd* pw = getpwnam(username);
            if (!pw) {
                std::cerr << "[Logger Error] Failed to get home directory for user: " << username << "\n";
                return;
            }

            std::string homeDir = pw->pw_dir;
            std::string logDir = homeDir + "/.local/share/DriveMgr/data/";
            struct stat st;
            if (stat(logDir.c_str(), &st) != 0) {
                if (mkdir(logDir.c_str(), 0755) != 0 && errno != EEXIST) {
                    std::cerr << RED << "[Logger Error] Failed to create log directory: " << logDir 
                            << " Reason: " << strerror(errno) << "\n";
                    return;
                }
            }

            std::string logPath = logDir + "log.dat";
            std::ofstream logFile(logPath, std::ios::app);
            if (logFile) {
                logFile << logMsg << std::endl;
            } else {
                std::cerr << RED << "[Logger Error] Unable to open log file: " << logPath
                        << " Reason: " << strerror(errno) << RESET <<"\n";
            }
        } else {
            return;
        }
    }
};


// TUI and runtime error

/**
 * @brief VERY IMPORTANT FOR TUI TO FUNCTION (DONT DELETE)
 */
extern struct termios oldt, newt;

/**
 * @brief Handles std::runtime_error exceptions by printing the error message to the standard error stream.
 * @param e The std::runtime_error exception to handle. (e.what() will be printed)
 * @note This function is intended to be used as a centralized error handling mechanism for runtime errors in the DriveMgr_CLI project.
 */
void handle_STD_Runtime_Error(const std::runtime_error& e) {
    std::cerr << "[RUNTIME_ERROR] " << e.what() << std::endl;
}

/**
 * @brief A helper function to throw a std::runtime_error with a given error message.
 *        This function also resets the terminal settings to ensure that the terminal is in a consistent state before throwing the error.
 * @param error_message The error message to include in the std::runtime_error exception.
 * @note This function should be used whenever a runtime error needs to be thrown, as it ensures that the terminal settings are properly reset before throwing the exception.
 */
void dmgr_runtime_error(const std::string& error_message) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\033[?1049l";
    throw std::runtime_error("[ERROR] " + error_message);
}


// Command executer

class Terminalexec {
public:
    /**
     * @brief Execute a shell command and return stdout
     * @param cmd C-string containing the command to execute
     * @return stdout output from the command
     * 
     * Forks a shell process and captures all stdout output.
     * Original version of the command execution wrapper.
     */
    static std::string execTerminal(const char* cmd) {
        ExecResult r = run_command(std::string(cmd));
        return r.stdout_str;
    }
    //v2 
    /**
     * @brief Execute a shell command and return stdout (v2)
     * @param command std::string containing the command to execute
     * @return stdout output from the command
     * 
     * Improved version using std::string parameter instead of C-string.
     * Captures both stdout and returns raw output without trimming.
     */
    static std::string execTerminalv2(const std::string &command) {
        ExecResult r = run_command(command);
        return r.stdout_str;
    }
    //v3
    /**
     * @brief Execute a shell command with error handling and output trimming (v3)
     * @param cmd std::string containing the command to execute
     * @return Trimmed stdout output, or empty string on error (exit_code != 0)
     * 
     * Latest version with error checking and automatic newline trimming.
     * Returns empty string if command fails, making it safe for direct use.
     */
    static std::string execTerminalv3(const std::string& cmd) {
        ExecResult r = run_command(cmd);
        if (r.exit_code != 0) return "";
        // Trim trailing newlines
        while (!r.stdout_str.empty() && (r.stdout_str.back() == '\n' || r.stdout_str.back() == '\r')) r.stdout_str.pop_back();
        return r.stdout_str;
    }
};

//recovery

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




// encryption
/// Path to encrypted key storage file - stores encryption keys for LUKS drives
const std::string KEY_STORAGE_PATH = std::string(getenv("HOME")) + "/.local/share/DriveMgr/data/key.bin";

/**
 * @struct EncryptionInfo
 * @brief Encryption metadata and keys for a drive
 * 
 * Stores the cryptographic material needed for AES-256-CBC encryption/decryption
 * of drive contents. Used in the encryption and decryption operations.
 */
struct EncryptionInfo {
    std::string driveName;      ///< Name identifier for the encrypted drive
    unsigned char key[32];      ///< 256-bit AES encryption key
    unsigned char iv[16];       ///< 128-bit Initialization Vector for CBC mode
};

#endif 
