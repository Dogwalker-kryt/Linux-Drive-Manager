#include "../include/DmgrLib.h"

// ========= Terminos IO =========

void TerminosIO::initiateTerminosInput() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
}

void TerminosIO::enableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void TerminosIO::restoreTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void TerminosIO::enableTerminosInput_diableAltTerminal() {
    initiateTerminosInput();
    std::cout << LEAVETERMINALSCREEN;
}


// ========= Logger =========

enum class LogType {
    ERROR,
    WARNING,
    INFO,
    SUCCESS,
    DRYRUN,
    EXEC
};

const char* Logger::logMessage(LogType log_type) {
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

void Logger::log(LogType type, const std::string& operation, const char* func) {
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

void Logger::error(const std::string &msg, const char* func) {
    log(LogType::ERROR, msg, func);
}

void Logger::warning(const std::string &msg, const char* func) {
    log(LogType::WARNING, msg, func);
}

void Logger::info(const std::string &msg, const char* func) {
    log(LogType::INFO, msg, func);
}

void Logger::success(const std::string &msg, const char* func) {
    log(LogType::SUCCESS, msg, func);
}

void Logger::dry_run(const std::string &msg, const char* func) {
    log(LogType::DRYRUN, msg, func);
}

void Logger::exec(const std::string &msg, const char* func) {
    log(LogType::EXEC, msg, func);
}

void Logger::clearLoggs(const std::string& path) {
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


// ========= helper/validtion/runtime error =========

std::string filePathHandler(const std::string &file_path) {
    const char* sudo_user = getenv("SUDO_USER");
    const char* user_env = getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;

    if (!username) {
        std::cerr << RED << "[ERROR] Could not determine username.\n" << RESET;
        LOG_ERROR("Could not determine username");
        return "";
    }

    const struct passwd* pw = getpwnam(username);

    if (!pw) {
        std::cerr << RED << "[ERROR] Could not get home directory for user: " << username << RESET << "\n";
        LOG_ERROR("Failed to get home directory for user: " + std::string(username));
        return "";
    }

    std::string homeDir = pw->pw_dir;
    std::string path = homeDir + file_path;
    
    return path;
}

void ldm_runtime_error(const std::string& error_message) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\033[?1049l";
    throw std::runtime_error("[ERROR] " + error_message);
}


// ========= input validation =========

std::string readLine() {
    std::string s;

    if (!std::getline(std::cin, s)) {
        
        ERR(ErrorCode::IOError, "std::getline failed");
        LOG_ERROR("std::getline() failed");
        return "";
    }

    return s;
}

namespace InputValidation {

    std::optional<int> getInt(const std::vector<int> &valid_ints) {
        std::string s_input = readLine();
        
        // Case 1: getline failed → s_input == "" AND stream is bad
        if (!std::cin.good()) {

            ERR(ErrorCode::IOError, "Failed to read input");
            return std::nullopt;

        }

        // Case 2: user entered empty line → s_input == "" but stream is fine
        if (s_input.empty()) {

            ERR(ErrorCode::InvalidInput, "Input cannot be empty");
            LOG_ERROR("Input is empty");
            return std::nullopt;

        }

        try {

            size_t idx = 0;
            int i_input = std::stoi(s_input, &idx);

            if (idx != s_input.size()) {

                ERR(ErrorCode::InvalidInput, "no characters can be used as input");
                LOG_ERROR("no characters can be used as input");
                return std::nullopt;

            }

            if (!valid_ints.empty() && std::find(valid_ints.begin(), valid_ints.end(), i_input) == valid_ints.end()) {

                ERR(ErrorCode::InvalidInput, "Input not in allowed integer list");
                LOG_ERROR("Input not in allowed integer list -> validateIntInput");
                return std::nullopt;

            }

            return i_input;

        } catch (const std::exception&) {

            ERR(ErrorCode::InvalidInput, "Conversion from string to int failed");
            LOG_ERROR("Conversion from string to int failed -> validateIntInput");
            return std::nullopt;

        }
    }

    std::optional<int> getInt(int min_value, int max_value) {
        auto val = getInt({});
        if (!val) return std::nullopt;

        if (*val < min_value || *val > max_value) {

            ERR(ErrorCode::InvalidInput, "Input outside allowed range");
            LOG_ERROR("Input outside allowed range");
            return std::nullopt;

        }

        return val;
    }

    std::optional<unsigned int> getUint() {
        std::string s_input = readLine();
        
        // Case 1: getline failed → s_input == "" AND stream is bad
        if (!std::cin.good()) {

            ERR(ErrorCode::IOError, "Failed to read input");
            return std::nullopt;

        }

        // Case 2: user entered empty line → s_input == "" but stream is fine
        if (s_input.empty()) {

            ERR(ErrorCode::InvalidInput, "Input cannot be empty");
            LOG_ERROR("Input is empty");
            return std::nullopt;

        }

        try {

            size_t idx = 0;
            unsigned long long tmp = std::stoull(s_input, &idx);

            if (idx != s_input.size()) {

                ERR(ErrorCode::InvalidInput, "only 1 character can be used as input");
                LOG_ERROR("no characters can be used as input");
                return std::nullopt;

            }

            if (tmp > std::numeric_limits<unsigned int>::max()) {
                ERR(ErrorCode::OutOfRange, "Number too large for unsigned int");
                LOG_ERROR("Number too large for unsigned int");
                return std::nullopt;
            }

            return static_cast<unsigned int>(tmp);

        } catch (const std::exception&) {

            ERR(ErrorCode::InvalidInput, "Conversion from string to uint failed");
            LOG_ERROR("Conversion from string to uint failed");
            return std::nullopt;

        }
    }

    std::optional<char> getChar(const std::vector<char> &valid_chars) {
        std::string s_input = readLine();

        if (!std::cin.good()) {

            ERR(ErrorCode::IOError, "Failed to read input");
            LOG_ERROR("std::getline() failed");
            return std::nullopt;

        }

        if (s_input.empty()) {

            ERR(ErrorCode::InvalidInput, "Input cannot be empty");
            LOG_ERROR("Input is empty");
            return std::nullopt;

        }

        auto start = s_input.find_first_not_of(" \t\n\r");
        auto end   = s_input.find_last_not_of(" \t\n\r");

        if (start == std::string::npos) {

            ERR(ErrorCode::InvalidInput, "Input cannot be empty or whitespace");
            LOG_ERROR("Input is empty or whitespace");
            return std::nullopt;

        }

        std::string trimmed = s_input.substr(start, end - start + 1);

        if (trimmed.size() != 1) {

            ERR(ErrorCode::InvalidInput, "Input must be exactly one character");
            LOG_ERROR("Input has more than one non-whitespace character");
            return std::nullopt;

        }

        char c_input = trimmed[0];

        // Validate allowed characters
        if (!valid_chars.empty() && std::find(valid_chars.begin(), valid_chars.end(), c_input) == valid_chars.end()) {

            ERR(ErrorCode::InvalidInput, "Character not allowed");
            LOG_ERROR("Char not in allowed list");
            return std::nullopt;

        }

        return c_input;
    }


}


// ==================== Side/Helper Functions ====================

std::string confirmationKeyGenerator() {
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

bool askForConfirmation(const std::string &prompt) {
    std::cout << prompt << "(y/n)\n";
    char confirm;
    std::cin >> confirm;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear inputbuffer

    if (confirm != 'Y' && confirm != 'y') {
        std::cout << BOLD << "[INFO] Operation cancelled\n" << RESET;
        LOG_INFO("Operation cancelled");
        return false;
    } 

    return true;
}

std::string removeFirstLines(const std::string& text, int n) {
    std::string out = text;
    for (int i = 0; i < n; i++) {
        size_t pos = out.find('\n');

        if (pos == std::string::npos)
            return out; // nothing left to remove

        out.erase(0, pos + 1);
    }
    return out;
}

void menuQues(bool& running) {   
    std::cout << BOLD <<"\nPress '1' for returning to the main menu, '2' to exit:\n" << RESET;

    auto menuques = InputValidation::getInt({1, 2});

    if (!menuques.has_value()) return;

    if (menuques == 1) {

        running = true;

    } else if (menuques == 2) {

        running = false;
    }
}

bool isRoot() {
    return (getuid() == 0);
}

bool checkRoot() {
    if (!isRoot()) {
        ERR(ErrorCode::PermissionDenied, "This function requires root privileges. Please run with 'sudo'");
        LOG_ERROR("Attempted to run without root privileges");
        return false;
    }
    return true;
}

bool checkRootMetadata() {
    if (!isRoot()) {
        std::cerr << YELLOW << "[WARNING] Running without root may limit functionality. For full access, please run with 'sudo'.\n" << RESET;
        LOG_WARNING("Running without root privileges");
        return false;
    }
    return true;
}
