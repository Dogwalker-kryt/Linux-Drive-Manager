#pragma once

#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <unordered_map>
#include <optional>


class EnvSys {
private:
    #define LEAVETERMINALSCREEN "\033[?1049l"
    #define PATH_MAX 4096

    static inline std::string getExecutablePath() {
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);

        if (len == -1) { 

            std::cout << LEAVETERMINALSCREEN;
            throw std::runtime_error("[EnvSys_ERROR] Failed to read /proc/self/exe");
    
        }
        buf[len] = '\0';
        return std::string(buf);
    }

    static inline std::filesystem::path computeAppRoot() {
        auto exe = std::filesystem::path(getExecutablePath());

        return exe.parent_path().parent_path().parent_path();
    }

public:
    static inline std::filesystem::path appRoot() {
        static std::filesystem::path root = computeAppRoot();
        
        return root;
    }
};
