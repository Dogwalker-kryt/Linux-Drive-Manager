/* −··− x f e t r o j k
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

// ! Warning this version is the experimental version of the program,
// This version has the latest and newest functions, but may contain bugs and errors
// Current version of this code is in the Info() function below
// v0.9.12.92_experimental

// standard C++ libraries, I think
#include <iostream>
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
#include <fstream>
#include <regex>
#include <map>
#include <cstdint>
#include <ctime>
#include <random>
#include <unordered_map>
// system
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <termios.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
// openssl
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
// custom .h
#include "../include/drivefunctions.h"


// ==================== global variables and definitions ====================

// Version
#define VERSION std::string("v0.9.12.92_experimental")

// TUI
struct termios oldt; 
struct termios newt;

// flags
static bool g_dry_run   =  false;
static bool g_no_color  =  false;
       bool g_no_log    =  false;

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

#define RESET   Color::reset()
#define RED     Color::red()
#define GREEN   Color::green()
#define YELLOW  Color::yellow()
#define BLUE    Color::blue()
#define MAGENTA Color::magenta()
#define CYAN    Color::cyan()
#define BOLD    Color::bold()
#define INVERSE Color::inverse()

static std::string THEME_COLOR      =   RESET;
static std::string SELECTION_COLOR  =   RESET;

// ==================== Command Execution Abstraction ====================

enum class ExecMode {
    NORMAL,      // Regular execution
    DRY_RUN,     // Show what would run
    QUIET        // No output to console, only logging
};

// Using existing ExecResult from command_exec.h
// Extended version with convenience members
struct CmdExecResult {
    bool success;
    std::string output;
    int exit_code;
};

class CmdExec {
private:
    static bool check_error(const std::string& output, const std::string& cmd) {
        if (output.find("error") != std::string::npos || 
            output.find("failed") != std::string::npos ||
            output.find("ERROR") != std::string::npos) {
            Logger::log("[EXEC] Command failed: " + cmd, g_no_log);
            return false;
        }
        return true;
    }

public:
    // Main command executor
    static CmdExecResult run(const std::string& cmd, bool use_sudo = false, ExecMode mode = ExecMode::NORMAL) {
        CmdExecResult result{false, "", -1};
        
        std::string final_cmd = cmd;
        if (use_sudo) final_cmd = "sudo " + cmd;
        
        // Dry-run mode
        if (g_dry_run || mode == ExecMode::DRY_RUN) {
            std::cout << YELLOW << "[DRY-RUN] Would execute: " << final_cmd << RESET << "\n";
            Logger::log("[DRY-RUN] " + final_cmd, g_no_log);
            result.success = true;
            return result;
        }
        
        // Execute via existing terminal functions
        result.output = Terminalexec::execTerminalv2(final_cmd.c_str());
        result.success = check_error(result.output, cmd);
        
        if (mode == ExecMode::QUIET) {
            // Don't print, only log
            Logger::log("[EXEC] " + cmd + " -> " + (result.success ? "OK" : "FAILED"), g_no_log);
        } else {
            // Normal mode: print output
            if (!result.output.empty()) std::cout << result.output << "\n";
        }
        
        return result;
    }
    
    // Convenience overloads
    static CmdExecResult run_sudo(const std::string& cmd, ExecMode mode = ExecMode::NORMAL) {
        return run(cmd, true, mode);
    }
    
    static CmdExecResult run_quiet(const std::string& cmd, bool use_sudo = false) {
        return run(cmd, use_sudo, ExecMode::QUIET);
    }
    
    // Check and throw on failure
    static CmdExecResult run_or_throw(const std::string& cmd, bool use_sudo = false) {
        CmdExecResult res = run(cmd, use_sudo);
        if (!res.success) {
            throw std::runtime_error("Command failed: " + cmd + "\n" + res.output);
        }
        return res;
    }
};

// Quick helpers for common patterns
#define EXEC(cmd)              CmdExec::run(cmd)
#define EXEC_SUDO(cmd)         CmdExec::run_sudo(cmd)
#define EXEC_QUIET(cmd)        CmdExec::run_quiet(cmd, false)
#define EXEC_QUIET_SUDO(cmd)   CmdExec::run_quiet(cmd, true)
#define EXEC_OR_THROW(cmd)     CmdExec::run_or_throw(cmd)


// ==================== Side/Helper Functions ====================

static std::vector<std::string> g_last_drives;

std::string listDrives(bool input_mode);

void checkDriveName(const std::string &driveName) {
    //ListDrives();
    listDrives(false);

    bool drive_found = false;
    for (const auto& drive : g_last_drives) {
        // match either full device path (/dev/sda) or the basename (sda)
        if (drive == driveName || std::filesystem::path(drive).filename() == std::filesystem::path(driveName).filename()) {
            drive_found = true;
            break;
        }
    }

    if (!drive_found) {
        std::cerr << RED << "[Error] Drive '" << driveName << "' not found!\n";
        Logger::log("[ERROR] Drive not found",g_no_log);
        return;
    }
}

std::string confirmationKeyGenerator() {
    const std::string chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    std::string displayKey;
    std::mt19937 gen(std::time(0));
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    for (int i = 0; i < 10; i++) {
        displayKey += chars[dist(gen)];
    }

    return displayKey;
}

bool askForConfirmation(const std::string &prompt) {
    std::cout << prompt << "(y/n)\n";
    char confirm;
    std::cin >> confirm;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear inputbuffer

    if (confirm != 'Y' && confirm != 'y') {
        std::cout << BOLD << "[INFO] Operation cancelled\n" << RESET;
        Logger::log("[INFO] Operation cancelled", g_no_color);
        return false;
    } 

    return true;
}

// keeping as fall back if problems arise with the TUI drive selection

// std::string getAndValidateDriveName(const std::string& prompt) {
//     ListDrives();

//     if (g_last_drives.empty()) {
//         std::cerr << RED << "[ERROR] No drives available to select!\n";
//         Logger::log("[ERROR] No drives available to select");
//         throw std::runtime_error("No drives available");
//     }

//     std::cout << "\n" << BOLD << prompt << ":" << RESET << "\n";
//     std::string driveName;
//     std::cin >> driveName;
//     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

//     if (driveName.empty()) {
//         std::cerr << RED << "[ERROR] Drive name cannot be empty!\n";
//         Logger::log("[ERROR] drive name cannot be empty\n");
//         throw std::runtime_error("Drive name cannot be empty");
//     }

//     if (size_t pos = driveName.find_first_of("--'&|<>;\""); pos != std::string::npos) {
//         std::cerr << RED << "[ERROR] Invalid characters in drive name!: " << pos << RESET << "\n";
//         Logger::log("[ERROR] Invalid characters in drive name\n");
//     }

//     if (driveName.find("/dev/") != 0) {
//         driveName = "/dev/" + driveName;
//     }

//     bool drive_found = false;
//     for (const auto& drive : g_last_drives) {
//         if (drive == driveName || std::filesystem::path(drive).filename() == std::filesystem::path(driveName).filename()) {
//             drive_found = true;
//             break;
//         }
//     }

//     if (!drive_found) {
//         std::cerr << RED << "[ERROR] Drive '" << driveName << "' not found!\n";
//         Logger::log("[ERROR] Drive not found");
//     }
//     return driveName;
// }

// void file_recovery_quick(const std::string& drive, const std::string& signature_key);
// void file_recovery_full(const std::string& drive, const std::string& signature_key);


// ==================== Main Logic Function and Classes ====================

std::string checkFilesystem(const std::string& device, const std::string& fstype) {
    if (fstype.empty()) return "Unknown filesystem";

    try {
        std::string cmd;
        if (fstype == "ext4" || fstype == "ext3" || fstype == "ext2") {
            cmd = "e2fsck -n " + device;
        } else if (fstype == "ntfs") {
            cmd = "ntfsfix --no-action " + device;
        } else if (fstype == "vfat" || fstype == "fat32") {
            cmd = "dosfsck -n " + device;
        }
        
        if (cmd.empty()) return "Unknown filesystem";
        
        auto result = EXEC_QUIET(cmd);
        
        if (result.output.find("clean") != std::string::npos || result.output.find("no errors") != std::string::npos) {
            return "Clean";
        } else if (!result.output.empty()) {
            return "Issues found";
        }

        return "Unknown state";

    } catch (const std::exception& e) {
        return "Check failed: " + std::string(e.what());
    }
}

// keeping as fall back if problems arise with the TUI drive selection

// // ListDrives
// void listDrives(std::vector<std::string>& drives) {
//     drives.clear();
//     auto res = EXEC("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p"); std::string lsblk = res.output;
//     std::cout << "\nAvailable Drives:\n";
//     std::cout << std::left 
//               << std::setw(3) << "#" 
//               << std::setw(18) << BOLD << "Device" << RESET 
//               << std::setw(10) << BOLD << "Size" << RESET 
//               << std::setw(10) << BOLD << "Type" << RESET
//               << std::setw(15) << BOLD << "Mountpoint" << RESET
//               << std::setw(10) << BOLD << "FSType" << RESET
//               << "Status" << std::endl;
//     std::cout << std::string(90, '-') << "\n";
//     std::istringstream iss(lsblk);
//     std::string line;
//     int idx = 0;

//     while (std::getline(iss, line)) {
//         if (line.find("disk") != std::string::npos) {
//             std::istringstream lss(line);
//             std::string device, size, type, mountpoint, fstype;
//             lss >> device >> size >> type;
            
//             std::string rest;
//             std::getline(lss, rest);
//             std::istringstream rss(rest);
//             if (rss >> mountpoint >> fstype) {
//                 if (mountpoint == "-") mountpoint = "";
//                 if (fstype == "-") fstype = "";
//             }
//             std::string status = checkFilesystem(device, fstype);
            
//             std::cout << std::left 
//                       << std::setw(3) << idx 
//                       << std::setw(18) << device 
//                       << std::setw(10) << size 
//                       << std::setw(10) << type 
//                       << std::setw(15) << (mountpoint.empty() ? "" : mountpoint)
//                       << std::setw(10) << (fstype.empty() ? "" : fstype)
//                       << status << std::endl;
                      
//             drives.push_back(device);
//             idx++;
//         }
//     }
//     if (drives.empty()) {
//         std::cerr << RED << "No drives found!\n" << RESET;
//         return;
//     }
// }

// TUI drive selection
std::string g_selected_drive;

std::string listDrives(bool input_mode) {
    static std::vector<std::string> drives;
    drives.clear();

    // Run lsblk using new CmdExec
    auto lsblk_res = EXEC_QUIET("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p");
    std::string lsblk = lsblk_res.output;

    // Print header
    std::cout << CYAN << "\nAvailable Drives:" << RESET << "\n";
    std::cout << std::left
              << std::setw(2) 
              << std::setw(5)  << "#"
              << BOLD << std::setw(18) << "Device"      << RESET
              << BOLD << std::setw(10) << "Size"        << RESET
              << BOLD << std::setw(10) << "Type"        << RESET
              << BOLD << std::setw(15) << "Mountpoint"  << RESET
              << BOLD << std::setw(10) << "FSType"      << RESET
              << "Status" << std::endl;

    std::cout << CYAN;
    std::cout << std::string(90, '-') << "\n" << RESET;

    // Parse lsblk output
    std::istringstream iss(lsblk);
    std::string line;
    int idx = 0;

    struct Row {
        std::string device, size, type, mount, fstype, status;
    };

    std::vector<Row> rows;

    while (std::getline(iss, line)) {
        if (line.find("disk") == std::string::npos) continue;

        std::istringstream lss(line);
        Row r;

        lss >> r.device >> r.size >> r.type;

        std::string rest;
        std::getline(lss, rest);
        std::istringstream rss(rest);

        rss >> r.mount >> r.fstype;
        if (r.mount == "-") r.mount = "";
        if (r.fstype == "-") r.fstype = "";

        r.status = checkFilesystem(r.device, r.fstype);

        // Print row
        std::cout << std::left
                  << std::setw(3)  << idx
                  << std::setw(18) << r.device
                  << std::setw(10) << r.size
                  << std::setw(10) << r.type
                  << std::setw(15) << r.mount
                  << std::setw(10) << r.fstype
                  << r.status << "\n";

        drives.push_back(r.device);
        rows.push_back(r);
        idx++;
    }

    if (drives.empty()) {
        std::cerr << RED << "No drives found!\n" << RESET;
        return "";
    }

    if (input_mode == true) {
        // enable raw mode
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);


        // --- INLINE TUI SELECTION ---
        int selected = 0;
        int total = drives.size();

        // Move cursor UP to the first drive row
        std::cout << "\033[" << total << "A";

        while (true) {
            for (int i = 0; i < total; i++) {
                std::cout << "\r"; // go to start of line

                // Arrow indicator
                if (i == selected)
                    std::cout << SELECTION_COLOR << "> " << RESET;
                else
                    std::cout << "  ";

                // Highlight row
                if (i == selected) std::cout << SELECTION_COLOR;

                std::cout << std::left
                        << std::setw(3)  << i
                        << std::setw(18) << rows[i].device
                        << std::setw(10) << rows[i].size
                        << std::setw(10) << rows[i].type
                        << std::setw(15) << rows[i].mount
                        << std::setw(10) << rows[i].fstype
                        << rows[i].status;

                if (i == selected) std::cout << RESET;

                std::cout << "\n"; 
            }
            // Move cursor back up
            std::cout << "\033[" << total << "A";
            
            // Read key
            char c;
            if (read(STDIN_FILENO, &c, 1) <= 0) continue;

            if (c == '\x1b') {
                char seq[2];
                if (read(STDIN_FILENO, &seq, 2) == 2) {
                    if (seq[1] == 'A') selected = (selected - 1 + total) % total; // up
                    if (seq[1] == 'B') selected = (selected + 1) % total;         // down
                }
            } else if (c == '\n' || c == '\r') {
                break; // Enter
            }
        }

        // Move cursor down past the table so next output prints normally
        int tableheight = total + 3;
        std::cout << "\033[" << tableheight << "B" << std::flush;
        std::cout << "\n";
        // restore terminal
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        g_selected_drive = drives[selected];
        return g_selected_drive;

    } else {
        return "";
    }
}

// Keep as fall back if problems arise with the TUI drive selection

// wrapper for listDrives [---- IGRNORE ----]
// void ListDrives() {
//     std::vector<std::string> drives;
//     listDrives(drives);

//     if (drives.empty()) {
//         std::cerr << RED << "No drives found!\n" << RESET;
//         Logger::log("[ERROR] No Drives found!");
//         return;
//     }

//     // Cache the last-listed drives so checkDriveName() can use them
//     g_last_drives = drives;
// }




// ==================== Partition Management ==================== 

class PartitionsUtils {
    public:
        // 1
        static bool resizePartition(const std::string& device, uint64_t newSizeMB) {
            try {
                std::string resize_partition_cmd = "parted --script " + device + " resizepart 1 " + 
                                 std::to_string(newSizeMB) + "MB";
                auto res = EXEC(resize_partition_cmd);
                return res.success;
            } catch (const std::exception&) {
                return false;
            }
        }

        // 2
        static bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB) {
            try {
                std::string move_partition_cmd = "parted --script " + device + " move " + 
                                 std::to_string(partNum) + " " + 
                                 std::to_string(startSectorMB) + "MB";
                auto res = EXEC_SUDO(move_partition_cmd);
                return res.success;
            } catch (const std::exception&) {
                return false;
            }
        }

        // 3
        static bool changePartitionType(const std::string& device, int partNum, const std::string& newType) {
            try {
                std::string backupCmd = "sfdisk -d " + device + " > " + device + "_backup.sf";
                EXEC_QUIET(backupCmd);

                std::string change_partition_cmd = "echo 'type=" + newType + "' | sfdisk --part-type " + 
                                 device + " " + std::to_string(partNum);
                auto res = EXEC(change_partition_cmd); std::string change_partition_cmd_output = res.output;
                return change_partition_cmd_output.find("error") == std::string::npos;
            } catch (const std::exception&) {
                return false;
            }
        }
};

void listpartisions() { 
    //std::string drive_name = getAndValidateDriveName("Enter the Name of the Drive you want to see the partitions of");
    std::string drive_name = listDrives(true); 

    std::cout << "\nPartitions of drive " << drive_name << ":\n";
    std::string list_partitions_cmd = "lsblk --ascii -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p " + drive_name;
    auto res = EXEC(list_partitions_cmd); std::string list_partitions_cmd_output = res.output;
    std::istringstream iss(list_partitions_cmd_output);
    std::string line;
    std::cout << std::left 
              << std::setw(25) << "Name" 
              << std::setw(10) << "Size" 
              << std::setw(10) << "Type" 
              << std::setw(15) << "Mountpoint"
              << "FSType" << "\n";
    std::cout << std::string(75, '-') << "\n";

    std::vector<std::string> partitions;
    while (std::getline(iss, line)) {
        if (line.find("part") != std::string::npos) {
            std::cout << line << "\n";
            std::istringstream lss(line);
            std::string part_name;
            lss >> part_name;
            partitions.push_back(part_name);
        }
    }

    if (partitions.empty()) {
        std::cout << RED << "[INFO] No partitions found on this drive.\n" << RESET;
        return;
    }
    std::cout << "\n┌─ Partition Management Options ─┐\n";
    std::cout << "├────────────────────────────────┤\n";
    std::cout << "│ 1. Resize partition            │\n";
    std::cout << "│ 2. Move partition              │\n";
    std::cout << "│ 3. Change partition type       │\n";
    std::cout << "│ 4. Return to main menu         │\n";
    std::cout << "└────────────────────────────────┘\n";
    std::cout << "Enter your choice: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
        case 1: {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;
            if (partNum < 1 || partNum > (int)partitions.size()) {
                std::cout << "Invalid partition number!\n";
                break;
            }

            std::cout << "Enter new size in MB: ";
            uint64_t newSize;
            std::cin >> newSize;
            if (newSize <= 0) {
                std::cout << "Invalid size!\n";
                break;
            }

            askForConfirmation("[Warning] Resizing partitions can lead to data loss.\nAre you sure? ");

            if (PartitionsUtils::resizePartition(partitions[partNum-1], newSize)) {
                std::cout << "Partition resized successfully!\n";
            } else {
                std::cout << "Failed to resize partition!\n";
                Logger::log("[ERROR] Failed to resize partition", g_no_log);
            }
            break;
            
        }
        case 2: {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;
            if (partNum < 1 || partNum > (int)partitions.size()) {
                std::cout << "Invalid partition number!\n";
                break;
            }

            std::cout << "Enter new start position in MB: ";
            uint64_t startPos;
            std::cin >> startPos;
            if (startPos < 0) {
                std::cout << "Invalid start position!\n";
                break;
            }

            askForConfirmation("[Warning] Moving partitions can lead to data loss.\nAre you sure? ");

            if (PartitionsUtils::movePartition(partitions[partNum-1], partNum, startPos)) {
                std::cout << "Partition moved successfully!\n";
            } else {
                std::cout << "Failed to move partition!\n";
                Logger::log("[ERROR] Failed to move partition", g_no_log);
            }
            break;

        }
        case 3: {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;

            if (partNum < 1 || partNum > (int)partitions.size()) {
                std::cout << "Invalid partition number!\n";
                break;
            }

            std::cout << "┌───────────────────────────┐\n";
            std::cout << "│ Available partition types │\n";
            std::cout << "├───────────────────────────┤\n";
            std::cout << "│ 1. Linux (83)             │\n";
            std::cout << "│ 2. NTFS (7)               │\n";
            std::cout << "│ 3. FAT32 (b)              │\n";
            std::cout << "│ 4. Linux swap (82)        │\n";
            std::cout << "└───────────────────────────┘\n";
            std::cout << "Enter type number: ";
            int typeNum;
            std::cin >> typeNum;
            std::string newType;
            switch (typeNum) {
                case 1: newType = "83"; break;
                case 2: newType = "7"; break;
                case 3: newType = "b"; break;
                case 4: newType = "82"; break;
                default:
                    std::cout << "Invalid type!\n";
                    break;
            }

            if (!newType.empty()) {
                askForConfirmation("[Warning] Changing partition type can make data inaccessible.\nAre you sure? ");
                if (PartitionsUtils::changePartitionType(drive_name, partNum, newType)) {
                    std::cout << "Partition type changed successfully!\n";
                } else {
                    std::cout << "Failed to change partition type!\n";
                    Logger::log("[ERROR] Failed to change partition type", g_no_log);
                }
            }
            break;

        }
        case 4:
            return;
        default:
            std::cout << "Invalid option!\n";
    }
}

// ==================== Disk Space Analysis ====================··−·
 
void analyDiskSpace() {
    //std::string drive_name = getAndValidateDriveName("Enter the Name of the Drive you want to analyze");
    std::string drive_name = listDrives(true); 

    std::cout  << CYAN << "\n------ Disk Information ------\n" << RESET;
    std::string analy_disk_space_cmd = "lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + drive_name;
    auto res = EXEC(analy_disk_space_cmd); std::string analy_disk_space_cmd_output = res.output;
    std::istringstream iss(analy_disk_space_cmd_output);
    std::string line;
    bool found = false;
    std::string mount_point;
    std::string size;

    while (std::getline(iss, line)) {
        std::istringstream lss(line);
        std::string name, type;
        lss >> name >> size >> type;
        std::getline(lss, mount_point);
        if (!mount_point.empty() && mount_point[0] == ' ') mount_point = mount_point.substr(1);

        if (type == "disk") {
            found = true;
            std::cout << "Device:      " << name << "\n";
            try {
                unsigned long long bytes = std::stoull(size);
                const char* units[] = {"B", "KB", "MB", "GB", "TB"};
                int unit = 0;
                double human_size = bytes;
                while (human_size >= 1024 && unit < 4) {
                    human_size /= 1024;
                    ++unit;
                }
                std::cout << "Size:        " << human_size << " " << units[unit] << "\n";
            } catch (...) {
                std::cout << "Size:        " << size << " bytes\n";
            }
            std::cout << "Type:        " << type << "\n";
            std::cout << "Mountpoint:  " << (mount_point.empty() ? "-" : mount_point) << "\n";
        }
    }

    if (!found) {
        std::cout << "No disk info found!\n";
    } else {
        if (!mount_point.empty() && mount_point != "-") {
            std::string df_cmd = "df -h '" + mount_point + "' | tail -1";
            auto res = EXEC(df_cmd); std::string df_out = res.output;
            std::istringstream dfiss(df_out);
            std::string filesystem, size, used, avail, usep, mnt;
            dfiss >> filesystem >> size >> used >> avail >> usep >> mnt;
            std::cout << "Used:        " << used << "\n";
            std::cout << "Available:   " << avail << "\n";
            std::cout << "Used %:      " << usep << "\n";
        } else {
            std::cout << "No mountpoint, cannot show used/free space.\n";
        }
    }

    std::cout << CYAN << "------------------------------\n" << RESET;
}


// ==================== Drive Formatting ====================

void formatDrive() {
    std::cout << "\n┌────── Choose an option for how to format ──────┐\n";
    std::cout << "├─────────────────────────────────────────────────┤\n";
    std::cout << "│ 1. Format drive                                 │\n";
    std::cout << "│ 2. Format drive with label                      │\n";
    std::cout << "│ 3. Format drive with label and filesystem       │\n";
    std::cout << "└─────────────────────────────────────────────────┘\n";
    int fdinput;
    std::cin >> fdinput;

    switch (fdinput) {
        case 1:
            std::cout << "Choose a Drive to Format\n";
            {
                //std::string driveName = getAndValidateDriveName("Enter name of a drive you want to format");
                std::string driveName = listDrives(true);

                std::cout << "Are you sure you want to format drive " << driveName << "? (y/n):\n";
                std::string confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd == "y" || confirmationfd == "Y" ) {
                    std::cout << "Formatting drive: " << driveName << "...\n";
                    std::string cmd = "mkfs.ext4 " + driveName;
                    auto res = EXEC(cmd); std::string result = res.output;
                    if (result.find("error") != std::string::npos) {
                        Logger::log("[ERROR] Failed to format drive: " + driveName, g_no_log);
                        std::cerr << RED << "[Error] Failed to format drive: " << driveName << RESET << "\n";
                    } else {
                        Logger::log("[INFO] Drive formatted successfully: " + driveName, g_no_log);
                        std::cout << GREEN << "Drive formatted successfully: " << driveName << RESET << "\n";
                    }
                } else {
                    Logger::log("[INFO] Format operation cancelled by user", g_no_log);
                    std::cout << "[Info] Format operation cancelled by user.\n";
                    return;
                }
            }
            break;

        case 2:
            {
                //std::string driveName = getAndValidateDriveName("Enter name of a drive you want to format with label");
                std::string driveName = listDrives(true);

                std::cout << "Enter label: ";
                std::string label;
                std::cin >> label;
                std::cout << "Formatting drive with label: " << label << "\n";
                if (label.empty()) {
                    std::cerr << RED << "[Error] label cannot be empty!\n" << RESET;
                    Logger::log("[ERROR] label cannot be empty", g_no_log);
                    return;
                }

                std::cout << "Are you sure you want to format drive " << driveName << " with label '" << label << "' ? (y/n)\n";
                char confirmationfd;
                std::cin >> confirmationfd;
                if (confirmationfd != 'y' && confirmationfd != 'Y') {
                    std::cout << BOLD << "[Info] Formatting cancelled!\n" << RESET; 
                    Logger::log("[INFO] Format operation cancelled by user", g_no_log);
                    break;
                }

                std::string execTerminalStr = ("mkfs.ext4 -L " + label + " " + driveName);
                auto res = EXEC(execTerminalStr); std::string execTerminal = res.output;
                if (execTerminal.find("error") != std::string::npos) {
                    std::cout << "[Error] Failed to format drive: " << driveName << "\n";
                    Logger::log("[ERROR] Failed to format drive: " + driveName, g_no_log);
                } else {
                    std::cout << "[INFO] Drive formatted successfully with label: " << label << "\n";
                    Logger::log("[INFO] Drive formatted successfully with label: " + label + " -> formatDrive()", g_no_log);
                }
            }
            break;

        case 3:
            {
                std::string driveName = listDrives(true);

                std::string label, fsType;
                std::cout << "Enter label: ";
                std::cin >> label;
                std::cout << "Enter filesystem type (e.g. ext4): ";
                std::cin >> fsType;
                std::cout << "Are you sure you want to format drive " << driveName << " with label '" << label << "' and filesystem type '" << fsType << "' ? (y/n)\n";
                char confirmation_fd3;
                std::cin >> confirmation_fd3;
                if (confirmation_fd3 != 'y' && confirmation_fd3 != 'Y') {
                    std::cout << "[Info] Formatting cancelled!\n";
                    Logger::log("[INFO] Format operation cancelled by user -> formatDrive()", g_no_log);
                    return;
                }

                std::string execTerminalStr = ("mkfs." + fsType + " -L " + label + " " + driveName.c_str());
                auto res = EXEC(execTerminalStr); std::string execTerminal = res.output;
                if (execTerminal.find("error") != std::string::npos) {
                    std::cerr << RED << "[Error] Failed to format drive: " << driveName << RESET << "\n";
                    Logger::log("[ERROR] Failed to format drive: " + driveName, g_no_log);
                    return;
                } else {
                    std::cout << "Drive formatted successfully with label: " << label << " and filesystem type: " << fsType << "\n";
                    Logger::log("[INFO] Drive formatted successfully with label: " + label + " and filesystem type: " + fsType + " -> formatDrive()", g_no_log);
                    return;
                }
            }
            break;

        default: {
            std::cerr << RED << "[Error] Invalid option selected.\n" << RESET;
            return;
        }
    }
}


// ==================== Drive Health Check ====================

int checkDriveHealth() {
    //std::string driveHealth_name = getAndValidateDriveName("Enter the name of the drive to check health");
    std::string driveHealth_name = listDrives(true);

    try {
        std::string check_drive_helth_cmd = "sudo smartctl -H " + driveHealth_name;
    auto res = EXEC(check_drive_helth_cmd); std::string check_drive_helth_cmd_output = res.output;
        std::cout << check_drive_helth_cmd_output;
    }
    catch(const std::exception& e) {
        std::string error = e.what();
        Logger::log("[ERROR]" + error, g_no_log);
        throw std::runtime_error(e.what());
    }
   return 0;
}


// ==================== Drive Resizing ====================

void resizeDrive() {
    //std::string driveName = getAndValidateDriveName("Enter the name of the drive to resize");
    std::string driveName = listDrives(true);

    std::cout << "Enter new size in GB for drive " << driveName << ":\n";
    int new_size;
    std::cin >> new_size;

    if (new_size <= 0) {
        std::cout << "[Error] Invalid size entered!\n";
        return;
    }

    std::cout << "Resizing drive " << driveName << " to " << new_size << " GB...\n";

    try {
        std::string resize_cmd = "sudo parted --script " + driveName +  " resizepart 1 " + std::to_string(new_size) + "GB";
        auto res = EXEC(resize_cmd); std::string resize_output = res.output;
        std::cout << resize_output << "\n";

        if (resize_output.find("error") != std::string::npos) {

            std::cout << RED << "[Error] Failed to resize drive: " << driveName << "\n" << RESET;
            Logger::log("[ERROR] Failed to resize drive: " + driveName + " -> resizeDrive()", g_no_log);

        } else {

            std::cout << GREEN << "Drive resized successfully\n" << RESET;
            Logger::log("[INFO] Drive resized successfully: " + driveName + " -> resizeDrive()", g_no_log);

        }
    } catch (const std::exception& e) {
        std::cerr << RED << "[ERROR] Exception during resize: " << e.what() << "\n" << RESET;
        Logger::log("[ERROR] Exception during resize: " + std::string(e.what()) + " -> resizeDrive()", g_no_log);
    }
}


// ==================== Drive Encryption Utilities ==================== 

class EnDecryptionUtils {
    public:
        static void saveEncryptionInfo(const EncryptionInfo& info) {
            std::ofstream file(KEY_STORAGE_PATH, std::ios::app | std::ios::binary);

            if (!file) {
                std::cerr << "[Error] Cannot open key storage file\n";
                Logger::log("[ERROR] Cannot open key storage file: " + KEY_STORAGE_PATH + " -> saveEncryptionInfo()", g_no_log);
                return;
            }

            auto salt = generateSalt();
            auto obfKey = obfuscate(info.key, sizeof(info.key), salt.data(), salt.size());
            auto obfIV = obfuscate(info.iv, sizeof(info.iv), salt.data(), salt.size());

            // Write drive name (null-terminated)
            char driveName[256] = {0};
            strncpy(driveName, info.driveName.c_str(), 255);
            file.write(driveName, sizeof(driveName));
            uint32_t saltLen = salt.size();

            file.write(reinterpret_cast<const char*>(&saltLen), sizeof(saltLen));
            file.write(reinterpret_cast<const char*>(salt.data()), salt.size());

            //·  − Write obfuscated key and IV
            file.write(reinterpret_cast<const char*>(obfKey.data()), obfKey.size());
            file.write(reinterpret_cast<const char*>(obfIV.data()), obfIV.size());

            file.close();
            Logger::log("[INFO] Encryption info saved (salted and obfuscated) for: " + info.driveName, g_no_log);
        }

        static bool loadEncryptionInfo(const std::string& driveName, EncryptionInfo& info) {
            std::ifstream file(KEY_STORAGE_PATH, std::ios::binary);

            if (!file) {
                std::cerr << RED << "[Error] Cannot open key storage file\n" << RESET;
                Logger::log("[ERROR] Cannot open key storage file: " + KEY_STORAGE_PATH + " -> loadEncryptionInfo()", g_no_log);
                return false;
            }

            char storedDriveName[256];
            while (file.read(storedDriveName, sizeof(storedDriveName))) {
                // Read salt length and salt
                uint32_t saltLen;
                file.read(reinterpret_cast<char*>(&saltLen), sizeof(saltLen));
                std::vector<unsigned char> salt(saltLen);
                file.read(reinterpret_cast<char*>(salt.data()), saltLen);

                // Read obfuscated key and IV
                std::vector<unsigned char> obfKey(32);
                std::vector<unsigned char> obfIV(16);
                file.read(reinterpret_cast<char*>(obfKey.data()), 32);
                file.read(reinterpret_cast<char*>(obfIV.data()), 16);

                // Deobfuscate
                auto key = deobfuscate(obfKey.data(), 32, salt.data(), saltLen);
                auto iv = deobfuscate(obfIV.data(), 16, salt.data(), saltLen);

                if (driveName == storedDriveName) {
                    std::copy(key.begin(), key.end(), info.key);
                    std::copy(iv.begin(), iv.end(), info.iv);
                    info.driveName = driveName;
                    Logger::log("[INFO] Encryption info loaded and deobfuscated for: " + driveName, g_no_log);
                    return true;
                }
            }
            Logger::log("[ERROR] No encryption info found for: " + driveName, g_no_log);
            return false;
        }

        static void generateKeyAndIV(unsigned char* key, unsigned char* iv) {
            if (!RAND_bytes(key, 32) || !RAND_bytes(iv, 16)) {
                throw std::runtime_error("[Error] Failed to generate random key/IV");
                Logger::log("[ERROR] Failed to generate random key/IV for encryption -> generateKeyAndIV()", g_no_log);
            }
        }

        static void encryptDrive(const std::string& driveName) {
            EncryptionInfo info;
            info.driveName = driveName;
            generateKeyAndIV(info.key, info.iv);
            saveEncryptionInfo(info);
            std::stringstream ss;

            // Write key to a secure temporary file and use it with cryptsetup
            std::string tmpKeyFile = "/tmp/drivemgr_key_" + std::to_string(getpid());
            {
                std::ofstream kf(tmpKeyFile, std::ios::binary | std::ios::trunc);
                if (!kf) {
                    std::cerr << RED << "[Error] Unable to create temporary key file\n" << RESET;
                    Logger::log("[ERROR] Unable to create temporary key file -> encryptDrive()", g_no_log);
                    return;
                }
                kf.write(reinterpret_cast<const char*>(info.key), 32);
            }
            chmod(tmpKeyFile.c_str(), S_IRUSR | S_IWUSR); // 0600

            ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
               << "--key-file " << tmpKeyFile << " open " << driveName
               << " encrypted_" << std::filesystem::path(driveName).filename().string();
            Logger::log("[INFO] Encrypting drive: " + driveName, g_no_log);
                auto res = EXEC_SUDO(ss.str());
                std::string output = res.output;

            // remove temp key file
            unlink(tmpKeyFile.c_str());

            if (!res.success || output.empty()) {
                std::cerr << RED << "[Error] Encryption failed: " << output << RESET << "\n";
                Logger::log("[ERROR] Encryption failed for drive: " + driveName + " -> encryptDrive()", g_no_log);
                return;
            }

            std::cout << "Drive encrypted successfully. The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
            Logger::log("[INFO] Drive encrypted successsfully: " + driveName + " -> encryptDrive()", g_no_log);
        }

        static void decryptDrive(const std::string& driveName) {
            EncryptionInfo info;

            if (!loadEncryptionInfo(driveName, info)) {
                std::cerr << RED << "[Error] No encryption key found for " << driveName << RESET << "\n";
                Logger::log("[ERROR] No encryption key found for " + driveName + " -> decryptDrive()", g_no_log);
                return;
            }

            // Create dm-crypt·−· mapping for decryption
            // write key to temp file securely
            std::string tmpKeyFile = "/tmp/drivemgr_key_" + std::to_string(getpid());
            {
                std::ofstream kf(tmpKeyFile, std::ios::binary | std::ios::trunc);
                if (!kf) {
                    std::cerr << RED << "[Error] Unable to create temporary key file\n" << RESET;
                    Logger::log("[ERROR] Unable to create temporary key file -> decryptDrive()", g_no_log);
                    return;
                }

                kf.write(reinterpret_cast<const char*>(info.key), 32);
            }

            chmod(tmpKeyFile.c_str(), S_IRUSR | S_IWUSR);

            std::stringstream ss;
            ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
               << "--key-file " << tmpKeyFile << " open " << driveName << " decrypted_"
               << std::filesystem::path(driveName).filename().string();
            auto res = EXEC_SUDO(ss.str());
            std::string output = res.output;

            // remove temp key file
            unlink(tmpKeyFile.c_str());

            if (!res.success || output.empty()) {
                std::cerr << RED << "Decryption failed: " << output << RESET << "\n";
                Logger::log("[ERROR] Decryption failed " + output + " -> decryptDrive()", g_no_log);
                return;
            }
            std::cout << GREEN << "Drive decrypted successfully.\n" << RESET;
            Logger::log("[INFO] Drive decrypted successfully -> decryptDrive()", g_no_log);
        }

        // salting
        static std::vector<unsigned char> generateSalt(size_t length = 16)  {
            std::vector<unsigned char> salt(length);

            if (!RAND_bytes(salt.data(), length)) {
                Logger::log("[ERROR] Failed to generate salt", g_no_log);
                throw std::runtime_error("[Error] Failed to generate salt");
            }
            return salt;
        }

        static std::vector<unsigned char> obfuscate(const unsigned char* data, size_t dataLen, const unsigned char* salt, size_t saltLen) {
            std::vector<unsigned char> result(dataLen);

            for (size_t i = 0; i < dataLen; ++i) {
                result[i] = data[i] ^ salt[i % saltLen];
            }
            return result;
        }

        static std::vector<unsigned char> deobfuscate(const unsigned char* data, size_t dataLen, const unsigned char* salt, size_t saltLen) {
            return obfuscate(data, dataLen, salt, saltLen);
        }
        
};

// ==================== Drive Encryption/Decryption main ====================

class DeEncrypting {
private:
    static void encrypting() {
        //std::string driveName = getAndValidateDriveName("Enter the NAME of a drive to encrypt:\n");
        std::string driveName = listDrives(true);

        std::cout << YELLOW << "[Warning] Are you sure you want to encrypt " << driveName << "? (y/n)\n" << RESET;
        char endecrypt_confirm;
        std::cin >> endecrypt_confirm;
        if (endecrypt_confirm != 'y' && endecrypt_confirm != 'Y') {
            std::cout << "[Info] Encryption cancelled.\n";
            Logger::log("[INFO] Encryption cancelled -> void EnDecrypt()", g_no_log);
            return;
        }

        std::string Key = confirmationKeyGenerator();

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << Key << "\n";
        std::string random_confirmationkey_input3;
        std::cin >> random_confirmationkey_input3;

        if (random_confirmationkey_input3 != Key) {
            std::cerr << RED << "[Error] Invalid confirmation of the Key or unexpected error\n" << RESET;
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> EnDecrypt()", g_no_log);
            return;
        }
        
        std::cout << BOLD << "[Process] Proceeding with encryption of " << driveName << "...\n" << RESET;
        EncryptionInfo info;
        info.driveName = driveName;
        
        if (!RAND_bytes(info.key, 32) || !RAND_bytes(info.iv, 16)) {
            std::cerr << RED << "[Error] Failed to generate encryption key\n" << RESET;
            Logger::log("[ERROR] Failed to generate encryption key -> void EnDecrypt()", g_no_log);
            return;
        }

        EnDecryptionUtils::loadEncryptionInfo(driveName, info);

        std::cout << "\n[Input] Enter a name for the encrypted device (e.g., encrypted_drive): ";
        std::string device_Name_Encrypt;
        std::cin >> device_Name_Encrypt;
        
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << device_Name_Encrypt;
        
        auto res = EXEC_SUDO(ss.str());
        std::string output = res.output;
        if (!res.success) {
            std::cerr << RED << "[Error] Failed to encrypt the drive: " << output << RESET << "\n";
            Logger::log("[ERROR] failed to encrypt the drive -> void EnDecrypt()", g_no_log);
        } else {
            std::cout << GREEN << "[Success] Drive " << driveName << " has been encrypted as " << device_Name_Encrypt << RESET << "\n";
            std::cout << "[Info] The decryption key is stored in " << KEY_STORAGE_PATH << "\n";
            Logger::log("[INFO] Key saved -> void EnDecrypt()", g_no_log);
            return;
        }
    }

    static void decrypting() {
        //std::string driveName = getAndValidateDriveName("Enter the NAME of a drive to decrypt:\n");
        std::string driveName = listDrives(true);

        std::cout << "[Warning] Are you sure you want to decrypt " << driveName << "? (y/n)\n";
        char decryptconfirm;
        std::cin >> decryptconfirm;

        if (decryptconfirm != 'y' && decryptconfirm != 'Y') {
            std::cout << "[Info] Decryption cancelled.\n";
            Logger::log("[INFO] Decryption cancelled -> void EnDecrypt()", g_no_log);
            return;
        }
        
        std::string Key = confirmationKeyGenerator();

        std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
        std::cout << Key << "\n";
        std::string random_confirmationkey_input2;
        std::cin >> random_confirmationkey_input2;

        if (random_confirmationkey_input2 != Key) {
            std::cerr << RED << "[Error] Invalid confirmation of the Key or unexpected error\n" << RESET;
            Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> EnDecryptDrive()", g_no_log);
            return;
        }
        
        std::cout << "[Process] Loading encryption key for " << driveName << "...\n";
        
        EncryptionInfo info;

        if (!EnDecryptionUtils::loadEncryptionInfo(driveName, info)) {
            std::cerr << RED << "[Error] No encryption key found for " << driveName << RESET << "\n";
            Logger::log("[ERROR] No encryption key found -> void EnDecrypt()", g_no_log);
            return;
        }
        
        std::cout << "Enter the name of the encrypted device to decrypt: ";
        std::string deviceNameDecrypt;
        std::cin >> deviceNameDecrypt;
    
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << driveName << " " << deviceNameDecrypt << "_decrypted";
        
        auto res = EXEC_SUDO(ss.str());
        std::string output = res.output;

        if (!res.success) {

            std::cerr << RED << "[Error] Failed to decrypt the drive: " << output << RESET << "\n";
            Logger::log("[ERROR] failed to decrypt the drive -> void EnDecrypt()", g_no_log);
            return;

        } else {
            std::cout << GREEN << "[Success] Drive " << driveName << " has been decrypted\n" << RESET;
            return;
        }
    }

public:
    static void main() {
        std::cout << "Do you want to 'e'ncrypt or 'd'ecrypt?:\n";
        char DeEncryption_main_input;
        std::cin >> DeEncryption_main_input;

        if (DeEncryption_main_input == 'e') {
            DeEncrypting::encrypting();

        } else if (DeEncryption_main_input == 'd') {
            DeEncrypting::decrypting();

        } else {
            std::cerr << RED << "[ERROR] Wrong input or unexpected error occurred\n" << RESET;
            return;
        }
    }
};


// ==================== Drive Data Overwriting ====================

void OverwriteDriveData() { 
    std::string drive_name_to_overwrite = listDrives(true);
    try {
        std::cout << YELLOW << "[WARNING]" << RESET << " Are you sure you want to overwrite all data on " << BOLD << drive_name_to_overwrite << RESET << " ? This actions cannot be undone! (y/n)";
        
        char overwrite_drive_confirmation;
        std::cin >> overwrite_drive_confirmation;

        if (overwrite_drive_confirmation != 'y' && overwrite_drive_confirmation != 'Y') {

            std::cout << BOLD << "[Overwriting aborted]" << RESET << " The Overwriting process of " << drive_name_to_overwrite << " was interupted by user\n";
            Logger::log("[INFO] Overwriting process aborted by user for drive: " + drive_name_to_overwrite, g_no_log);
            return;

        }

        std::cout << "To be sure you want to overwrite the data on " << drive_name_to_overwrite << " you need to enter the following safety key:\n";

        std::string confirmation_key = confirmationKeyGenerator();
        Logger::log("[INFO] Confirmation key generated for overwriting drive: " + drive_name_to_overwrite, g_no_log);

        std::cout << confirmation_key;

        std::cout << "Enter the confirmation key:\n";

        std::string confirmation_key_user_input;
        std::cin >> confirmation_key_user_input;

        if (confirmation_key_user_input != confirmation_key) {

            std::cout << BOLD << "[INFO]" << RESET << " The confirmationkey was incorrect, the overwriting process has been interupted\n";
            Logger::log("[INFO] Incorrect confirmation key entered, overwriting process aborted for drive: " + drive_name_to_overwrite, g_no_log);
            return;

        }

        std::cout << YELLOW << "[Process]" << RESET << " Proceeding with overwriting all data on: " << drive_name_to_overwrite << "\n";

        try {
            auto res_urandom = EXEC_SUDO("dd if=/dev/urandom of=" + drive_name_to_overwrite + " bs=1M status=progress && sync"); std::string dev_random_output = res_urandom.output;
            auto res_zero = EXEC_SUDO("dd if=/dev/zero of=" + drive_name_to_overwrite + " bs=1M status=progress && sync"); std::string dev_zero_output = res_zero.output;
            
            if (!res_urandom.success && !res_zero.success) {

                std::cout << RED << "[ERROR] Failed to overwrite the drive: " << drive_name_to_overwrite << "\n" << RESET;
                Logger::log("[INFO] Overwriting failed to complete for drive: " + drive_name_to_overwrite, g_no_log);
                return;

            } else if (!res_urandom.success || !res_zero.success) {

                std::cout << YELLOW << "[Warning]" << RESET << " One of the overwriting operations failed, but the drive may have been partially overwritten. Please check the output and try again if necessary.\n";
                Logger::log("[WARNING] One of the overwriting operations failed for drive: " + drive_name_to_overwrite, g_no_log);
                return;
            } 

            std::cout << GREEN << "[Success]" << RESET << " Overwriting completed successfully for drive: " << drive_name_to_overwrite << "\n";
            Logger::log("[INFO] Overwriting completed successfully for drive: " + drive_name_to_overwrite, g_no_log);
            return;

        } catch (const std::exception& e) {

            Logger::log("[ERROR] Failed during overwriting process for drive: " + drive_name_to_overwrite + " Reason: " + e.what(), g_no_log);
            std::cerr << RED << "[ERROR] Failed during overwriting process: " << e.what() << RESET << "\n";
            return;

        }

    } catch (const std::exception& e) {

        std::cerr << RED << "[ERROR] Failed to initialize Overwriting function: " << e.what() << RESET << "\n";
        Logger::log("[ERROR] Failed to initialize Overwriting function: " + std::string(e.what()), g_no_log);
        return;
    }
}


// ==================== Drive Metadata Reader ====================

class MetadataReader {
private:
    struct DriveMetadata {
        std::string name;
        std::string size;
        std::string model;
        std::string serial;
        std::string type;
        std::string mountpoint;
        std::string vendor;
        std::string fstype;
        std::string uuid;
    };

    static DriveMetadata getMetadata(const std::string& drive) {
        DriveMetadata metadata;

        std::string cmd = "lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p " + drive;

        auto res = EXEC_QUIET(cmd); std::string json = res.output;
        size_t deviceStart = json.find("{", json.find("["));
        size_t childrenPos = json.find("\"children\"", deviceStart);

        std::string deviceBlock = json.substr(deviceStart, childrenPos - deviceStart);

        auto extractValue = [&](const std::string& key, const std::string& from) -> std::string {
            std::regex pattern("\"" + key + "\"\\s*:\\s*(null|\"(.*?)\")");
            std::smatch match;
            
            if (std::regex_search(from, match, pattern)) {
                if (match[1] == "null")
                    return "";
                else
                    return match[2].str();
            }
            return "";
        };

        metadata.name       = extractValue("name", deviceBlock);
        metadata.size       = extractValue("size", deviceBlock);
        metadata.model      = extractValue("model", deviceBlock);
        metadata.serial     = extractValue("serial", deviceBlock);
        metadata.type       = extractValue("type", deviceBlock);
        metadata.mountpoint = extractValue("mountpoint", deviceBlock);
        metadata.vendor     = extractValue("vendor", deviceBlock);
        metadata.fstype     = extractValue("fstype", deviceBlock);
        metadata.uuid       = extractValue("uuid", deviceBlock);
        return metadata;
    }

    static void displayMetadata(const DriveMetadata& metadata) {
        std::cout << "\n-------- Drive Metadata --------\n";
        std::cout << "| Name:       " << metadata.name << "\n";
        std::cout << "| Size:       " << metadata.size << "\n";
        std::cout << "| Model:      " << (metadata.model.empty() ? "N/A" : metadata.model) << "\n";
        std::cout << "| Serial:     " << (metadata.serial.empty() ? "N/A" : metadata.serial) << "\n";
        std::cout << "| Type:       " << metadata.type << "\n";
        std::cout << "| Mountpoint: " << (metadata.mountpoint.empty() ? "Not mounted" : metadata.mountpoint) << "\n";
        std::cout << "| Vendor:     " << (metadata.vendor.empty() ? "N/A" : metadata.vendor) << "\n";
        std::cout << "| Filesystem: " << (metadata.fstype.empty() ? "N/A" : metadata.fstype) << "\n";
        std::cout << "| UUID:       " << (metadata.uuid.empty() ? "N/A" : metadata.uuid) << "\n";

        // SMART data
        if (metadata.type == "disk") {
            std::cout << "\n┌-─-─-─- SMART Data -─-─-─-─\n";
            std::string smartCmd = "sudo smartctl -i " + metadata.name;
            auto res = EXEC(smartCmd); std::string smartOutput = res.output;


            if (!smartOutput.empty()) {
                std::cout << smartOutput;
            } else {
                std::cout << "[ERROR] SMART data not available/intalled\n";
            }
        }   

        std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";         
    } 
    
public:
    static void mainReader() {
        // ListDrives();
        // std::cout << "\nEnter Drive Name for reading metadata\n";
        // std::string driveName;
        // std::cin >> driveName;
        // checkDriveName(driveName);
        try{
            //std::string driveName = getAndValidateDriveName("Enter Drive name for reading metadata");
            std::string driveName = listDrives(true);

            try {

                DriveMetadata metadata = getMetadata(driveName);
                displayMetadata(metadata);
                Logger::log("[INFO] Successfully read metadata for drive: " + driveName, g_no_log);

            } catch (const std::exception& e) {

                std::cout << RED << "[ERROR] Failed to read drive metadata: " << e.what() << RESET << "\n";
                Logger::log("[ERROR] Failed to read drive metadata: " + std::string(e.what()), g_no_log);
                return;

            }
        } catch (const std::exception& e) {

            std::cerr << RED << "[ERROR] Failed to initialize metadata reading" << RESET << "\n";
            return;
        }
    }
};


// ==================== Mounting and Burning Utilities ====================

class MountUtility {
private:
    static void BurnISOToStorageDevice() { //const std::string& isoPath, const std::string& drive
        try {
            //std::string driveName = getAndValidateDriveName("[Burn ISO/IMG] Enter the Name of a drive you want to burn an iso/img to:");
            std::string driveName = listDrives(true);

            std::cout << "Are you sure you want to select " << driveName << " for this operation? (y/n)\n";
            char confirmation_burn;
            std::cin >> confirmation_burn;

            if (confirmation_burn != 'y' && confirmation_burn != 'Y') {
                std::cout << "[Info] Operation cancelled\n";
                Logger::log("[INFO] Operation cancelled -> BurnISOToStorageDevice()", g_no_log);
                return;
            }

            std::cout << "\nEnter the path to the iso/img file you want to burn on " << driveName << ":\n";
            std::string isoPath;
            std::cin >> isoPath;

            std::cout << "Are you sure you want to burn " << isoPath << " to " << driveName << "? (y/n)\n";
            char confirmation_burn2;
            std::cin >> confirmation_burn2;

            if (confirmation_burn2 != 'y' && confirmation_burn2 != 'Y') {
                std::cout << "[Info] Operation cancelled\n";
                Logger::log("[INFO] Operation cancelled -> BurnISOToStorageDevice()", g_no_log);
                return;
            }

            char random_confirmationkey[] = {'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H', 'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P', 'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X', 'y', 'Y', 'z', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
            for (int i = 0; i < 10; i++) {
                int randomIndex = rand() % (sizeof(random_confirmationkey) / sizeof(random_confirmationkey[0]));
                std::cout << random_confirmationkey[randomIndex];
            }

            std::cout << "\nPlease enter the confirmationkey to proceed with the operation:\n";
            std::cout << random_confirmationkey << "\n";
            char random_confirmationkey_input[10];
            std::cin >> random_confirmationkey_input;

            if (std::string(random_confirmationkey_input) != std::string(random_confirmationkey)) {

                std::cout << "[Error] Invalid confirmation of the Key or unexpected error\n";
                Logger::log("[ERROR] Invalid confirmation of the Key or unexpected error -> OverwriteData", g_no_log);
                return;

            } else {
                try {

                    std::cout << "Proceeding with burning " << isoPath << " to " << driveName << "...\n";
                    std::string bruncmd = "sudo dd if=" + isoPath + " of=" + driveName + " bs=4M status=progress && sync";
                    auto res = EXEC_SUDO(bruncmd); std::string brunoutput = res.output;

                    if (brunoutput.find("error") != std::string::npos) {
                        Logger::log("[ERROR] Failed to burn iso/img to drive: " + driveName + " -> BurnISOToStorageDevice()", g_no_log); 
                        throw std::runtime_error("[Error] Failed to burn iso/img to drive: " + driveName);
                    }

                    std::cout << "[Success] Successfully burned " << isoPath << " to " << driveName << "\n";
                    Logger::log("[INFO] Successfully burned iso/img to drive: " + driveName + " -> BurnISOToStorageDevice()", g_no_log);
                
                } catch (const std::exception& e) {
                    std::cout << e.what() << "\n";
                } 
            }
        } catch (const std::exception& e) {

            std::cout << RED << "[ERROR] Failed to initialize burn iso/img: " << e.what() << RESET << "\n";
            Logger::log("[ERROR] Failed to burn iso/img: " + std::string(e.what()), g_no_log);
            return;
        }
    }

    static void MountDrive2() {
        try{
            //std::string driveName = getAndValidateDriveName("[Mount] Enter the Name of a drive you want to mount:");
            std::string driveName = listDrives(true);

            std::string mountpoint = "sudo mount " + driveName + " /mnt/" + std::filesystem::path(driveName).filename().string();
            auto res = EXEC_SUDO(mountpoint); std::string mountoutput = res.output;

            if (mountoutput.find("error") != std::string::npos) {
                std::cout << "[Error] Failed to mount drive: " << mountoutput << "\n";
                Logger::log("[ERROR] Failed to mount drive: " + driveName + " -> MountDrive()", g_no_log);
                return;
            }

        } catch (const std::exception& e) {
            std::cerr << RED << "[ERROR] Failed to initialize mount disk: " << e.what() << RESET << "\n";
            Logger::log("[ERROR] Failed to initialize mount disk: " + std::string(e.what()), g_no_log);
            return;
        }
    }

    static void UnmountDrive2() {
        try {
            //std::string driveName = getAndValidateDriveName("[Unmount] Enter the Name of a drive you want to unmount:");
            std::string driveName = listDrives(true );

            std::string unmountpoint = "sudo umount " + driveName;
            auto res = EXEC_SUDO(unmountpoint); std::string unmountoutput = res.output;

            if (unmountoutput.find("error") != std::string::npos) {
                std::cerr << RED << "[ERROR] Failed to unmount drive: " << unmountoutput << "\n";
                Logger::log("[ERROR] Failed to unmount drive: " + driveName + " -> UnmountDrive()", g_no_log);
                return;
            }

        } catch (const std::exception& e) {

            std::cerr << RED << "[ERROR] Failed to initialize disk unmounting: " << e.what() << "\n";
            Logger::log("[ERROR] Failed to initialize disk unmounting: " + std::string(e.what()), g_no_log);
            return; 
        }
    }

    static void Restore_USB_Drive() {
        std::string restore_device_name = listDrives(true);
        try {

            std::cout << "Are you sure you want to overwrite/clean the ISO/Disk_Image from: " << restore_device_name << " ? [y/N]\n";
            char confirm = 'n';
            std::cin >> confirm;

            if (std::tolower(confirm) != 'y') {
                std::cout << "Restore process aborted\n";
                return;
            }

            // Unmount
            EXEC_QUIET_SUDO("umount " + restore_device_name + "* 2>/dev/null || true");
            
            // Wipe filesystem
            auto wipefs_res = EXEC_SUDO("wipefs -a " + restore_device_name);
            if (!wipefs_res.success) {
                std::cerr << RED << "[Error] Failed to wipe device: " << restore_device_name << RESET << "\n";
                return;
            }

            // Zero out start
            auto dd_res = EXEC_SUDO("dd if=/dev/zero of=" + restore_device_name + " bs=1M count=10 status=progress && sync");
            if (!dd_res.success) {
                Logger::log("[ERROR] Failed to overwrite the iso image on the usb -> Restore_USB_Drive()", g_no_log);
                std::cerr << RED << "[Error] Failed to overwrite device: " << restore_device_name << RESET << "\n";
                return;
            }

            // Create partition table
            auto parted_res = EXEC_SUDO("parted -s " + restore_device_name + " mklabel msdos mkpart primary 1MiB 100%");
            if (!parted_res.success) {
                Logger::log("[ERROR] Failed while restoring USB: " + restore_device_name, g_no_log);
                std::cerr << RED << "[Error] Partition table creation failed.\n" << RESET;
                return;
            }

            // Probe partitions
            auto partprobe_res = EXEC_SUDO("partprobe " + restore_device_name);

            std::string partition_path = restore_device_name;
            if (!partition_path.empty() && std::isdigit(partition_path.back())) 
                partition_path += "p1"; 
            else 
                partition_path += "1";
            
            // Format as FAT32
            auto mkfs_res = EXEC_SUDO("mkfs.vfat -F32 " + partition_path);
            if (!mkfs_res.success) {
                Logger::log("[ERROR] Failed while formatting USB: " + restore_device_name, g_no_log);
                std::cerr << RED << "[Error] Filesystem creation failed.\n" << RESET;
                return;
            }

            std::cout << GREEN << "[Success] Your USB should now function as a normal FAT32 drive (partition: " << partition_path << ")\n" << RESET;
            Logger::log("[INFO] Restored USB device " + restore_device_name + " -> formatted " + partition_path, g_no_log);
            return;

        } catch (const std::exception& e) {

            std::cerr << RED << "[ERROR] Failed to initialize usb restore function: " << e.what() << RESET << "\n";
            Logger::log("[ERROR] failed to initialize restore usb function", g_no_log);
            return;
        }
    }

    static int ExitReturn(bool& running) {
        running = false;
        return 0;   
    }

public:
    static void mainMountUtil() {
        enum MenuOptions {
            Burniso = 1, MountDrive = 2, UnmountDrive = 3, Exit = 0, RESTOREUSB = 4
        };

        std::cout << "\n--------- Mount menu ---------\n";
        std::cout << "1. Burn iso/img to storage device\n";
        std::cout << "2. Mount storage device\n";
        std::cout << "3. Unmount storage device\n";
        std::cout << "4. Restore usb from iso\n";
        std::cout << "0. Rreturn to main menu\n";
        std::cout << "--------------------------------\n";

        int menuinputmount;
        std::cin >> menuinputmount;
        switch (menuinputmount) {
            case Burniso: {
                BurnISOToStorageDevice();
                break;  
            }

            case MountDrive: {
                MountDrive2();
                break;
            }

            case UnmountDrive: {
                UnmountDrive2();
                break;
            }

            case RESTOREUSB: {
                Restore_USB_Drive();
                break;
            }

            case Exit: {
                return;
            }

            default: {
                std::cout << "[Error] Invalid selection\n";
                return;
            }
        }
    }
};


// ==================== Forensic Analysis Utilities ====================

class ForensicAnalysis {
private:
    static int exit() {
        return 0;
    }

    static void CreateDiskImage() {
        try {
            std::string driveName = listDrives(true);

            std::cout << "Enter the path where the disk image should be saved (e.g., /path/to/image.img):\n";
            std::string imagePath;
            std::cin >> imagePath;
            std::cout << "Are you sure you want to create a disk image of " << driveName << " at " << imagePath << "? (y/n)\n";
            char confirmationcreate;
            std::cin >> confirmationcreate;

            if (confirmationcreate != 'y' && confirmationcreate != 'Y') {
                std::cout << "[Info] Operation cancelled\n";
                Logger::log("[INFO] Operation cancelled -> CreateDiskImage()", g_no_log);
                return;
            }

            auto res = EXEC_SUDO("dd if=" + driveName + " of=" + imagePath + " bs=4M status=progress && sync");
            if (!res.success) {
                std::cout << RED << "[Error] Failed to create disk image\n" << RESET;
                Logger::log("[ERROR] Failed to create disk image for drive: " + driveName + " -> CreateDiskImage()", g_no_log);
                return;
            }

            std::cout << GREEN << "[Success] Disk image created at " << imagePath << "\n" << RESET;
            Logger::log("[INFO] Disk image created successfully for drive: " + driveName + " -> CreateDiskImage()", g_no_log);
       
        } catch (const std::exception& e) {
            std::cerr << RED << "[ERROR] Failed to create Disk image: " << e.what() << RESET << "\n";
            Logger::log("[ERROR] Failed to create disk image: " + std::string(e.what()), g_no_log);
            return;
        }
    }

    // recoverymain + side functions
    static void recovery() {
        std::cout << "\n-------- Recovery ---------−−−\n";
        std::cout << "1. files recovery\n";
        std::cout << "2. partition recovery\n";
        std::cout << "3. system recovery\n";
        std::cout << "--------------------------------\n";
        std::cout << "Enter your choice:\n"; 
        int scanDriverecover;
        std::cin >> scanDriverecover;

        switch (scanDriverecover) {
            case 1: {
                filerecovery();
                break;
            }

            case 2: {
                partitionrecovery();
                break;
            }

            case 3: {
                systemrecovery();
                break;
            }

            default: {
                std::cout << "[Error] invalid input\n";  
                break;     
            }  
        }   
    }

    //·−−− recovery side functions
    static void filerecovery() {
        //std::string device = getAndValidateDriveName("Enter the NAME of a drive or image to scan for recoverable files (e.g., /dev/sda:");
        std::string device = listDrives(true);

        static const std::vector<std::string> signature_names = {"all","png","jpg","elf","zip","pdf","mp3","mp4","wav","avi","tar.gz","conf","txt","sh","xml","html","csv"};
        std::cout << "Type signature to search (e.g. png) or 'all':\n";
        std::string sig_in;
        std::cin >> sig_in;
        size_t sig_idx = SIZE_MAX;

        for (size_t i = 0; i < signature_names.size(); ++i) if (signature_names[i] == sig_in) { sig_idx = i; break; }
        if (sig_idx == SIZE_MAX) { std::cout << "[Error] Unsupported signature\n"; return; }

        std::cout << "Scan depth: 1=quick 2=full\n";
        int depth = 0;
        std::cin >> depth;

        if (depth == 1) file_recovery_quick(device, (int)sig_idx);
        else if (depth == 2) file_recovery_full(device, (int)sig_idx);
        else std::cout << "[Error] Invalid depth\n";
    }

    static void file_recovery_quick(const std::string& drive, int signature_type) {
        std::cout << "Scanning drive for recoverable files (quick) - signature index: " << signature_type << "...\n";

        // Mapping of the numeric menu choices to signature keys
        static const std::vector<std::string> signature_names = {
            "all", "png", "jpg", "elf", "zip", "pdf", "mp3", "mp4", "wav", "avi",
            "tar.gz", "conf", "txt", "sh", "xml", "html", "csv"
        };

        if (signature_type < 0 || static_cast<size_t>(signature_type) >= signature_names.size()) {
            std::cout << "[Error] Invalid signature type index.\n";
            return;
        }

        const std::string key = signature_names[signature_type];

        // Helper: scan a signature over the drive, limited to max_blocks (SIZE_MAX = full)
        auto scan_signature = [&](const file_signature& sig, size_t max_blocks) {
            if (sig.header.empty()) return;

            const size_t block_size = 4096;
            std::ifstream disk(drive, std::ios::binary);

            if (!disk.is_open()) {
                std::cerr << "[Error] Cannot open drive/image: " << drive << "\n";
                return;
            }

            std::vector<uint8_t> prev_tail;
            size_t offset = 0; // bytes read so far
            size_t blocks_read = 0;
            const size_t header_len = sig.header.size();

            while (disk && (max_blocks == SIZE_MAX || blocks_read < max_blocks)) {
                std::vector<char> buf(block_size);
                disk.read(buf.data(), block_size);
                std::streamsize n = disk.gcount();
                if (n <= 0) break;

                // window = prev_tail + buf
                std::vector<uint8_t> window;
                window.reserve(prev_tail.size() + static_cast<size_t>(n));
                window.insert(window.end(), prev_tail.begin(), prev_tail.end());
                window.insert(window.end(), reinterpret_cast<uint8_t*>(buf.data()), reinterpret_cast<uint8_t*>(buf.data()) + n);

                // search for header in window
                for (size_t i = 0; i + header_len <= window.size(); ++i) {
                    if (std::memcmp(window.data() + i, sig.header.data(), header_len) == 0) {
                        size_t found_offset = offset + i - prev_tail.size();
                        std::cout << "[FOUND] ." << sig.extension << " signature at offset: " << found_offset << "\n";
                    }
                }

                // keep the last (header_len - 1) bytes to handle signatures spanning blocks
                if (header_len > 1) {
                    size_t tail_len = std::min(window.size(), header_len - 1);
                    prev_tail.assign(window.end() - tail_len, window.end());
                } else {
                    prev_tail.clear();
                }

                offset += static_cast<size_t>(n);
                ++blocks_read;
            }

            disk.close();
        };

        if (key == "all") {
            // quick: limit to first N blocks per signature to stay fast
            const size_t quick_blocks = 1024; // ~4MB
            for (const auto& kv : signatures) {
                std::cout << "Quick scanning for: " << kv.first << "\n";
                scan_signature(kv.second, quick_blocks);
            }

        } else {

            auto it = signatures.find(key);
            if (it == signatures.end()) {
                std::cout << "[Error] Signature not found: " << key << "\n";
                return;
            }
            scan_signature(it->second, 1024);
        }
    }

    static void file_recovery_full(const std::string& drive, int signature_type) {
        std::cout << "Scanning drive for recoverable files (full) - signature index: " << signature_type << "...\n";

        static const std::vector<std::string> signature_names = {
            "all", "png", "jpg", "elf", "zip", "pdf", "mp3", "mp4", "wav", "avi",
            "tar.gz", "conf", "txt", "sh", "xml", "html", "csv"
        };

        if (signature_type < 0 || static_cast<size_t>(signature_type) >= signature_names.size()) {
            std::cout << "[Error] Invalid signature type index.\n";
            return;
        }

        const std::string key = signature_names[signature_type];

        auto scan_signature_full = [&](const file_signature& sig) {
            if (sig.header.empty()) return;
            const size_t block_size = 4096;
            std::ifstream disk(drive, std::ios::binary);

            if (!disk.is_open()) {
                std::cerr << "[Error] Cannot open drive/image: " << drive << "\n";
                return;
            }

            std::vector<uint8_t> prev_tail;
            size_t offset = 0;
            const size_t header_len = sig.header.size();

            while (disk) {
                std::vector<char> buf(block_size);
                disk.read(buf.data(), block_size);
                std::streamsize n = disk.gcount();
                if (n <= 0) break;

                std::vector<uint8_t> window;
                window.reserve(prev_tail.size() + static_cast<size_t>(n));
                window.insert(window.end(), prev_tail.begin(), prev_tail.end());
                window.insert(window.end(), reinterpret_cast<uint8_t*>(buf.data()), reinterpret_cast<uint8_t*>(buf.data()) + n);

                for (size_t i = 0; i + header_len <= window.size(); ++i) {
                    if (std::memcmp(window.data() + i, sig.header.data(), header_len) == 0) {
                        size_t found_offset = offset + i - prev_tail.size();
                        std::cout << "[FOUND] ." << sig.extension << " signature at offset: " << found_offset << "\n";
                    }
                }

                if (header_len > 1) {
                    size_t tail_len = std::min(window.size(), header_len - 1);
                    prev_tail.assign(window.end() - tail_len, window.end());
                } else {
                    prev_tail.clear();
                }

                offset += static_cast<size_t>(n);
            }

            disk.close();
        };

        if (key == "all") {
            for (const auto& kv : signatures) {
                std::cout << "Full scanning for: " << kv.first << "\n";
                scan_signature_full(kv.second);
            }

        } else {

            auto it = signatures.find(key);
            if (it == signatures.end()) {
                std::cout << "[Error] Signature not found: " << key << "\n";
                return;
            }
            scan_signature_full(it->second);
        }
    }

    static void partitionrecovery() {
        std::string device = listDrives(true);

        std::cout << "\n--- Partition table (parted) ---\n";
        auto parted_res = EXEC_SUDO("parted -s " + device + " print");
        
        std::cout << "\n--- fdisk -l ---\n";
        auto fdisk_res = EXEC_SUDO("fdisk -l " + device + " 2>/dev/null || true");

        // Offer to dump partition table using sfdisk (non-destructive)
        std::cout << "Would you like to save a partition-table dump (recommended) to a file for possible restoration? (y/N): ";
        char saveDump = 'n';
        std::cin >> saveDump;
        std::string dumpPath;

        if (saveDump == 'y' || saveDump == 'Y') {
            dumpPath = device;

            // sanitize filename: replace '/' with '_'
            for (auto &c : dumpPath) if (c == '/') c = '_';
            dumpPath = "/tmp/" + dumpPath + "_sfdisk_dump.sfdisk";
            
            std::string dump_cmd = "sfdisk -d " + device + " > " + dumpPath + " 2>&1";
            auto dump_res = EXEC_SUDO(dump_cmd);

            // Check if file was created
            std::ifstream fcheck(dumpPath);
            if (fcheck) {

                std::cout << "Partition-table dump written to: " << dumpPath << "\n";
                Logger::log("[INFO] Partition-table dump saved: " + dumpPath + " for device: " + device + " -> partitionrecovery()", g_no_log);
           
            } else {

                std::cout << "[Warning] Could not write partition-table dump.\n";
                Logger::log("[WARN] Failed to write partition-table dump for device: " + device + " -> partitionrecovery()", g_no_log);
            }
        }

        std::cout << "\nNotes:\n";
        std::cout << " - The tool printed the partition table above. If you see missing partitions, you can try recovery tools such as 'testdisk' or restore a saved sfdisk dump with 'sudo sfdisk " << device << " < " << (dumpPath.empty() ? "<dump-file>" : dumpPath) << "'.\n";
        std::cout << " - 'testdisk' is interactive; run it manually if you want a guided recovery.\n";

        // Check for testdisk availability and offer to run guidance only
        auto testdisk_res = EXEC_QUIET_SUDO("which testdisk 2>/dev/null || true");

        if (!testdisk_res.output.empty()) {
            std::cout << "\nDetected 'testdisk' on the system. This is an interactive tool that can help recover partitions.\n";
            std::cout << "I will not run it automatically. To run it now, open a terminal and run: sudo testdisk " << device << "\n";
        } else {
            std::cout << "\n'testdisk' not found. You can install it (usually package 'testdisk') to perform interactive partition recovery.\n";
        }

        std::cout << "\nPartition recovery helper finished. Review outputs and saved dump before attempting destructive actions.\n";
    }
    
    static void systemrecovery() {
        std::string device = listDrives(true);

        std::cout << "\nListing partitions and filesystems for " << device << "\n";
        std::string lsblk_cmd = "lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT,LABEL -p -n " + device;
        auto lsblk_res = EXEC(lsblk_cmd);
        
        std::cout << "\nProbing for possible boot partitions (EFI and Linux root candidates)...\n";

        // Find an EFI partition (vfat with esp flag) and a Linux root (ext4/xfs/btrfs)
        auto blkid_res = EXEC_SUDO("blkid -o export " + device + "* 2>/dev/null || true");

        // Also show partition flags from parted
        auto parted_res = EXEC_SUDO("parted -s " + device + " print");

        std::cout << "\nIf you want to attempt automatic repair of the bootloader, DriveMgr will prepare a script with suggested steps (it will not run it unless you explicitly allow execution).\n";
        // Build a suggested script (dry-run by default)
        std::string scriptPath = "/tmp/drive_mgr_repair_";
        std::string sanitized = device;
        for (auto &c : sanitized) if (c == '/') c = '_';
        scriptPath += sanitized + ".sh";

        std::ofstream script(scriptPath);
        if (!script) {
            std::cout << "[Error] Could not create helper script at " << scriptPath << "\n";
            Logger::log("[ERROR] Could not create system recovery script: " + scriptPath + " -> systemrecovery()", g_no_log);
            return;
        }

        script << "#!/bin/sh\n";
        script << "# DriveMgr generated helper script: inspect and run manually or allow DriveMgr to run with explicit confirmation.\n";
        script << "# Device: " << device << "\n";
        script << "set -e\n";
        script << "echo 'This script will attempt to mount root and reinstall grub. Inspect before running.'\n";
        script << "# Example sequence (adapt to your partition layout):\n";
        script << "# 1) Mount root partition: sudo mount /dev/sdXY /mnt\n";
        script << "# 2) If EFI: sudo mount /dev/sdXZ /mnt/boot/efi\n";
        script << "# 3) Bind system dirs: sudo mount --bind /dev /mnt/dev && sudo mount --bind /proc /mnt/proc && sudo mount --bind /sys /mnt/sys\n";
        script << "# 4) chroot and reinstall grub: sudo chroot /mnt grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ubuntu || sudo chroot /mnt grub-install /dev/sdX\n";
        script << "# 5) update-grub inside chroot: sudo chroot /mnt update-grub\n";
        script << "echo 'Script created for guidance only. Do not run without verifying paths.'\n";
        script.close();

        auto chmod_res = EXEC("chmod +x " + scriptPath);

        std::cout << "A helper script was created at: " << scriptPath << "\n";
        std::cout << "Open and inspect it. If you want DriveMgr to attempt to run the helper script now, type the exact phrase 'I UNDERSTAND' (all caps) to confirm: ";
        std::string confirmation;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, confirmation);

        if (confirmation == "I UNDERSTAND") {
            std::cout << "Running helper script (requires sudo). This is destructive if incorrect.\n";
            Logger::log("[WARN] User allowed DriveMgr to run system recovery helper script: " + scriptPath + " -> systemrecovery()", g_no_log);
            auto run_res = EXEC_SUDO("sh " + scriptPath);
            std::cout << "Helper script finished. Inspect system state manually.\n";
        } else {
            std::cout << "Not running the helper script. Inspect and run manually if desired: sudo sh " << scriptPath << "\n";
        }

    }
    //−·−
public:
    static void mainForensic(bool& running) {
        enum ForensicMenuOptions {
            Info = 1, CreateDisktImage = 2, ScanDrive = 3, Exit_Return = 0
        };

        std::cout << "\n┌─────── Forensic Analysis menu ────────┐\n";
        std::cout << "│ 1. Info of the Analysis tool            │\n";
        std::cout << "│ 2. Create a disk image of a drive       │\n";
        std::cout << "│ 3. recover of system, files,..          │\n";                                                                                                                                                                                                                                                                                                                                                                                                        
        std::cout << "│ 0. Exit/Return to the main menu         │\n";                                                                                                                                                                                                                                                                                                                                                                                               
        std::cout << "└─────────────────────────────────────────┘\n";
        int forsensicmenuinput;
        std::cin >> forsensicmenuinput;

        switch (static_cast<ForensicMenuOptions>(forsensicmenuinput)) {
            case Info: {
                std::cout << BOLD << "\n[Info] This is a custom made forensic analysis tool for the Drive Manager\n";
                std::cout << "Its not using actual forsensic tools, but still if its finished would be fully functional\n";
                std::cout << "In development...\n" << RESET;
                break;
            }

            case CreateDisktImage: {
                CreateDiskImage();
                break;
            }

            case ScanDrive: {
                recovery();
                break;
            }

            case Exit_Return: {
                std::cout << "\nDo you want to (r)eturn to the main menu or (e)xit? (r/e)\n";
                char exitreturninput;
                std::cin >> exitreturninput;

                if (exitreturninput == 'r' || exitreturninput == 'R') return;
                else if (exitreturninput == 'e' || exitreturninput == 'E') {
                    running = false;
                    exit();
                }

                break;
                return;
            }

            default: {
                std::cout << "[Error] Invalid selection\n";
                return;
            }
        }
    }
};


// ==================== Drive Structure Visualization (DSV) ====================

class DSV {
private:
    static long getSize(const std::string &path) {
        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) == 0) {
            return statbuf.st_size;
        }
        return 0;
    }

    static void listFirstLayerFolders(const std::string &path) {
        DIR *dp = opendir(path.c_str());
        if (!dp) {
            std::cerr << "Error opening directory: " << path << '\n';
            return;
        }

        std::cout << "\n| " << std::setw(30) << std::left << "Name";
        std::cout << " | " << std::setw(10) << "Size";
        std::cout << " | Visualization\n";
        std::cout << std::string(60, '-') << std::endl;

        struct dirent *entry;
        while ((entry = readdir(dp)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    std::string fullPath = path + "/" + entry->d_name;
                    long size = getSize(fullPath);
                    std::cout << "| " << std::setw(30) << std::left << entry->d_name;
                    std::cout << " | " << std::setw(10) << size / (1024 * 1024) << " MB |";
                    //std::cout << std::string(20, '') << "\n";
                }
            }
        }
        closedir(dp);
    }

public:
    static void DSVmain() {
        //std::string driveName = getAndValidateDriveName("Enter the name of a drive to visualize its contents");
        std::string driveName = listDrives(true);

        std::string currentPath = "/"; 
        listFirstLayerFolders(currentPath);

        while (true) {
            std::cout << "Enter folder to explore, '..' to go up, or 'exit' to quit: ";
            std::string input;
            std::cin >> input;

            if (input == "exit") {
                break;
            } else if (input == "..") {
                size_t pos = currentPath.find_last_of('/');
                if (pos != std::string::npos) {
                    currentPath = currentPath.substr(0, pos);
                }
            } else {

                std::string nextPath = currentPath + input;

                if (getSize(nextPath) > 0) {
                    currentPath = nextPath;
                    listFirstLayerFolders(currentPath);
                } else {
                    std::cout << "Directory does not exist or cannot be accessed.\n";
                }

            }
        }
    }
};


// ==================== Clone Drive Utility ====================

class Clone {
    private:
        static void CloneDrive(const std::string &source, const std::string &target) {
            std::cout << "\n[CloneDrive] Do you want to clone data from " << source << " to " << target << "? This will overwrite all data on the target drive(n) (y/n): ";
            char confirmation = 'n';
            std::cin >> confirmation;

            if (confirmation != 'y' && confirmation != 'Y') {
                std::cout << "[Info] Operation cancelled\n";
                Logger::log("[INFO] Operation cancelled -> Clone::CloneDrive()", g_no_log);
                return;

            } else if (confirmation == 'y' || confirmation == 'Y') {
                try {

                    auto res = EXEC_SUDO("dd if=" + source + " of=" + target + " bs=4M status=progress && sync");
                    if (!res.success) {
                        Logger::log("[ERROR] Failed to clone drive from " + source + " to " + target + " -> Clone::CloneDrive()", g_no_log);
                        throw std::runtime_error("Clone operation failed");
                    }

                    std::cout << GREEN << "[Success] Drive cloned from " << source << " to " << target << "\n" << RESET;
                    Logger::log("[INFO] Drive cloned successfully from " + source + " to " + target + " -> Clone::CloneDrive()", g_no_log);
                
                } catch (const std::exception& e) {

                    std::cout << RED << "[ERROR] Failed to clone drive: " << e.what() << RESET << "\n";
                    Logger::log(std::string("[ERROR] Failed to clone drive from ") + source + " to " + target + " -> Clone::CloneDrive(): " + e.what(), g_no_log);
                    return;
                }
            }

        }
        
    public:
        static void mainClone() {
            try {
                //std::string sourceDrive = getAndValidateDriveName("Enter the source drive name to clone");
                std::string sourceDrive = listDrives(true);

                std::cout << "\nEnter the target drive name to clone the data on to:\n";
                std::string targetDrive;
                std::cin >> targetDrive;
                checkDriveName(targetDrive);

                if (sourceDrive == targetDrive) {

                    Logger::log("[ERROR] Source and target drives are the same -> Clone::mainClone()", g_no_log);
                    throw std::runtime_error("Source and target drives cannot be the same!");
                    return;

                } else {

                    CloneDrive(sourceDrive, targetDrive);
                    return;
                }
            } catch (std::exception& e) {

                std::cerr << RED << "[ERROR] " << e.what() << RESET << "\n";
                Logger::log("[ERROR] " + std::string(e.what()), g_no_log);
                return; // Return to menu
            }
        }
};


// ==================== Log Viewer Utility ====================

void logViewer() {
    const char* sudo_user = getenv("SUDO_USER");
    const char* user_env = getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;

    if (!username) {
        std::cerr << RED << "[Error] Could not determine username.\n" << RESET;
        return;
    }

    struct passwd* pw = getpwnam(username);

    if (!pw) {
        std::cerr << RED << "[Error] Could not get home directory for user: " << username << RESET << "\n";
        return;
    }

    std::string homeDir = pw->pw_dir;
    std::string path = homeDir + "/.local/share/DriveMgr/data/log.dat";
    std::ifstream file(path);

    if (!file) {
        Logger::log("[ERROR] Unable to read log file at " + path, g_no_log);
        std::cerr << RED << "[Error] Unable to read log file at path: " << path << RESET << "\n";
        return;
    }

    std::cout << "\nLog file content:\n";
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }
}


// ==================== Configuration Editor Utility ====================

std::string configPath() {
    const char* sudo_user = std::getenv("SUDO_USER");
    const char* user_env = std::getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;

    if (!username) {
        std::cerr << RED << "[Config Error] Could not determine username.\n" << RESET;
        return ""; // return empty string on error
    }

    struct passwd* pw = getpwnam(username);

    if (!pw) {
        std::cerr << RED << "[Config Error] Failed to get home directory for user: " << username << "\n" << RESET;
        return "";
    }

    std::string homeDir = pw->pw_dir;
    return homeDir + "/.local/share/DriveMgr/data/config.conf";
}


struct CONFIG_VALUES {
    std::string UI_MODE;
    std::string COMPILE_MODE;
    std::string ROOT_MODE;
    std::string THEME_COLOR_MODE;
    std::string SELECTION_COLOR_MODE;
    bool DRY_RUN_MODE;
};

CONFIG_VALUES configHandler() {
    CONFIG_VALUES cfg{}; 
    
    std::string conf_file = configPath();
    if (conf_file.empty()) {
        return cfg;
    }

    std::ifstream config_file(conf_file);
    if (!config_file.is_open()) {
        Logger::log("[Config_handler ERROR] Cannot open config file", g_no_log);
        std::cerr << RED << "[Config_handler ERROR] Cannot open config file\n" << RESET;
        return cfg;
    }

    std::string line;
    while (std::getline(config_file, line)) {

        if (line.empty() || line[0] == '#')
            continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);

        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "UI_MODE") cfg.UI_MODE = value;
        else if (key == "COMPILE_MODE") cfg.COMPILE_MODE = value;
        else if (key == "ROOT_MODE") cfg.ROOT_MODE = value;
        else if (key == "COLOR_THEME") cfg.THEME_COLOR_MODE = value;
        else if (key == "SELECTION_COLOR") cfg.SELECTION_COLOR_MODE = value;
        else if (key == "DRY_RUN_MODE") {
            std::string v = value;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            cfg.DRY_RUN_MODE = (v == "true");
        }

    }
    return cfg;
}

bool fileExists(const std::string& path) { struct stat buffer; return (stat(path.c_str(), &buffer) == 0); }

void configEditor() {
    extern struct termios oldt, newt;
    CONFIG_VALUES cfg = configHandler();
    
    std::cout<< "┌─────" << BOLD << " config values " << RESET << "─────┐\n";
    std::cout << "│ UI mode: "          << cfg.UI_MODE               << "\n";
    std::cout << "│ Compile mode: "     << cfg.COMPILE_MODE          << "\n";
    std::cout << "│ Root mode: "        << cfg.ROOT_MODE             << "\n";
    std::cout << "│ Theme Color: "      << cfg.THEME_COLOR_MODE      << "\n";
    std::cout << "│ Selection Color: "  << cfg.SELECTION_COLOR_MODE  << "\n";
    std::cout << "└─────────────────────────┘\n";   
   
    std::cout << "\n";
    std::cout << "Do you want to edit the config file? (y/n)\n";
    std::string config_edit;
    std::cin >> config_edit;
    
    if (config_edit == "n" || config_edit.empty()) {
        return;
        
    } else if (config_edit == "y") {
        const char* sudo_user = std::getenv("SUDO_USER");
        const char* user_env = std::getenv("USER");
        const char* username = sudo_user ? sudo_user : user_env;
        
        if (!username) {
            std::cerr << "[Config Editor Error] Could not determine username.\n";
            return;
        }

        struct passwd* pw = getpwnam(username);
        if (!pw) {
            std::cerr << "[Config Editor Error] Failed to get home directory for user: " << username << "\n";
            return;
        }

        std::string homeDir = pw->pw_dir;

        std::string lumePath = homeDir + "/.local/share/DriveMgr/bin/Lume/Lume";

        if (!fileExists(lumePath)) {
            std::cerr << RED << "[Config Editor Error] Lume editor not found at:" << lumePath << "\n" << RESET;
            return;
        }

        std::string configPath = homeDir + "/.local/share/DriveMgr/data/config.conf";
        std::string config_editor_cmd = "\"" + lumePath + "\" \"" + configPath + "\"";

        // Leave alt screen
        std::cout << "\033[?1049l" << std::flush;

        // Restore terminal to normal mode
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        // Run Lume
        system(config_editor_cmd.c_str());

        // Re-enter alt screen
        std::cout << "\033[?1049h" << std::flush;

        // Re-enable raw mode
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        return;
    }        
}

// if you add any colors then you must add them here!!! very important
std::string avilable_colores[6] = { "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN"};
std::string color_codes[6] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};

void colorThemeHandler() {
    CONFIG_VALUES cfg = configHandler();
    std::string color_theme_name = cfg.THEME_COLOR_MODE;
    std::string selection_theme_name = cfg.SELECTION_COLOR_MODE;

    size_t count = sizeof(avilable_colores) / sizeof(avilable_colores[0]);

    // color Theme handler
    THEME_COLOR = RESET;

    for (size_t i = 0; i < count; i++) {
        if (avilable_colores[i] == color_theme_name) {
            THEME_COLOR = color_codes[i];
            break;
        }
    }

    // Selection color handler
    SELECTION_COLOR = RESET;

    for (size_t i = 0; i < count; i++) {
        if (avilable_colores[i] == selection_theme_name) {
            SELECTION_COLOR = color_codes[i];
            break;
        }
    }
}


// ==================== Drive Benchmark Utility ====================

double benchmarkSequentialWrite(const std::string& path, size_t totalMB = 512) {
    const size_t blockSize = 4 * 1024 * 1024; // 4MB blocks
    std::vector<char> buffer(blockSize, 'A');

    std::string tempFile = path + "/dmgr_benchmark.tmp";
    std::ofstream out(tempFile, std::ios::binary);

    if (!out.is_open()) return -1;

    auto start = std::chrono::high_resolution_clock::now();

    size_t written = 0;
    while (written < totalMB * 1024 * 1024) {
        out.write(buffer.data(), blockSize);
        written += blockSize;
    }

    out.close();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    double mbps = (double)totalMB / seconds;

    std::remove(tempFile.c_str());
    return mbps;
}

double benchmarkSequentialRead(const std::string& path, size_t totalMB = 512) {
    const size_t blockSize = 4 * 1024 * 1024;

    std::string tempFile = path + "/dmgr_benchmark.tmp";
    {
        std::ofstream out(tempFile, std::ios::binary);
        std::vector<char> buffer(blockSize, 'A');
        for (size_t i = 0; i < totalMB * 1024 * 1024; i += blockSize)
            out.write(buffer.data(), blockSize);
    }

    std::ifstream in(tempFile, std::ios::binary);
    if (!in.is_open()) return -1;

    std::vector<char> buffer(blockSize);

    auto start = std::chrono::high_resolution_clock::now();

    while (in.read(buffer.data(), blockSize)) {}

    auto end = std::chrono::high_resolution_clock::now();
    in.close();

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    double mbps = (double)totalMB / seconds;

    std::remove(tempFile.c_str());
    return mbps;
}

double benchmarkRandomRead(const std::string& path, size_t operations = 50000) {
    const size_t blockSize = 4096;

    std::string tempFile = path + "/dmgr_benchmark.tmp";
    {
        std::ofstream out(tempFile, std::ios::binary);
        std::vector<char> buffer(blockSize, 'A');
        for (size_t i = 0; i < operations * blockSize; i += blockSize)
            out.write(buffer.data(), blockSize);
    }

    std::ifstream in(tempFile, std::ios::binary);
    if (!in.is_open()) return -1;

    std::vector<char> buffer(blockSize);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < operations; i++) {
        size_t offset = (rand() % operations) * blockSize;
        in.seekg(offset);
        in.read(buffer.data(), blockSize);
    }

    auto end = std::chrono::high_resolution_clock::now();
    in.close();

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();

    std::remove(tempFile.c_str());
    return operations / seconds; // IOPS
}

struct Rule {
    std::function<bool(double,double,double)> match;
    std::string label;
};

std::string classifyDrive(double w, double r, double iops) {
    std::vector<Rule> rules = {
        { [](double w, double r, double i){ return w > 0 && r > 0; },
          "The Benchmark failed\n" },

        { [](double w, double r, double i){ return w < 50 && r < 200; },
          "Your device has the performance of a USB Flash Drive\n" },

        { [](double w, double r, double i){ return w < 150 && r < 150; },
          "Your device has the performance of a Hard Disk Drive (HDD)\n" },

        { [](double w, double r, double i){ return w < 600 && r < 600; },
          "Your device has the performance of a SATA SSD\n" },

        { [](double w, double r, double i){ return w < 3500 && r < 3500; },
          "Your device has the performance of an NVMe SSD (Gen3)\n" },

        { [](double w, double r, double i){ return w >= 3500; },
          "Your device has the performance of a high‑performance NVMe SSD (Gen4/Gen5)\n" }
    };

    for (auto& rule : rules)
        if (rule.match(w, r, iops))
            return rule.label;

    return "[ERROR] Program failed to classify your Drive type\nDrive type: Unkown\n";
}

void printBenchmarkSpeeds(double wirte_speed_seq, double read_speed_seq, double iops_speed) {
    std::string w_str = std::to_string(wirte_speed_seq);
    std::string r_str = std::to_string(read_speed_seq);
    std::string i_str = std::to_string(iops_speed);

    auto fmt = [](double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
    };

    std::string w_str2 = fmt(wirte_speed_seq);
    std::string r_str2 = fmt(read_speed_seq);
    std::string i_str2 = fmt(iops_speed);

    const int innerWidth = 30;

    int pad_w = innerWidth - (2 + w_str2.length());
    int pad_r = innerWidth - (2 + r_str2.length());
    int pad_i = innerWidth - (2 + i_str2.length());



    // std::cout << "┌────────────────────────────────┐\n";
    // std::cout << "│       Benchmark results        │\n";
    // std::cout << "├────────────────────────────────┤\n";
    // std::cout << "│                                │\n";
    // std::cout << "├──   Seqential Write speed    ──┤\n";
    // std::cout << "│: " << w_str << std::setw(pad_w) <<"│\n";
    // std::cout << "│                                │\n";
    // std::cout << "├──   Sequential Read speed    ──┤\n";
    // std::cout << "│: " << r_str << std::setw(pad_r) << "│\n";
    // std::cout << "├──         IOPS speed         ──┤\n";
    // std::cout << "│: " << i_str << std::setw(pad_i) << "│\n";
    // std::cout << "└──────────────────────────── \n";
    std::cout << BOLD << "\n[Benchmark results]\n" << RESET;
    std::cout << " Sequential Write speed : " << w_str2 << " MB/s\n";
    std::cout << " Sequential Read speed  : " << r_str2 << " MB/s\n";
    std::cout << " IOPS speed             : " << i_str2 << " IOPS\n";

    std::string aprox_drive_type = classifyDrive(wirte_speed_seq, read_speed_seq, iops_speed);
    std::cout << aprox_drive_type;
}

void benchmarkMain() {
    std::cout << BOLD << "\n[Drive Benchmark Tool]\n" << RESET;
    std::cout << "Enter a path like '/home', '/mnt/drive' or 'media/[user]/usb_device' to benchmark\n";
    std::string benchmark_drive_name;

    std::cout << "Make sure you have around 1 GB free space on the Drive\n";
    std::cout << "And dont kill the program during the Benchmark, this can cause the temp benchmark file to be not deleted!";
    std::cin >> benchmark_drive_name;

    double write_speed_seq = benchmarkSequentialWrite(benchmark_drive_name);
    double read_speed_seq = benchmarkSequentialRead(benchmark_drive_name);
    double iops_speed = benchmarkRandomRead(benchmark_drive_name);

    printBenchmarkSpeeds(write_speed_seq, read_speed_seq, iops_speed);
}


// ==================== Drive Fingerprinting Utility ====================

class DriveFingerprinting {
private:
    struct DriveMetadata {
        std::string name;
        std::string size;
        std::string model;
        std::string serial;
        std::string uuid;
    };

    static DriveMetadata getMetadata(const std::string& drive) {
        DriveMetadata metadata;
        std::string cmd = "lsblk -J -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -p " + drive;
        auto res = EXEC_QUIET(cmd);
        std::string json = res.output;
        
        size_t deviceStart = json.find("{", json.find("["));
        size_t childrenPos = json.find("\"children\"", deviceStart);
        
        if (deviceStart == std::string::npos || childrenPos == std::string::npos) {
            Logger::log("[WARN] Could not parse metadata JSON for drive: " + drive, g_no_log);
            return metadata;
        }
        
        std::string deviceBlock = json.substr(deviceStart, childrenPos - deviceStart);

        auto extractValue = [&](const std::string& key, const std::string& from) -> std::string {
            std::regex pattern("\"" + key + "\"\\s*:\\s*(null|\"(.*?)\")");
            std::smatch match;

            if (std::regex_search(from, match, pattern)) {
                if (match[1] == "null")
                    return "";
                else
                    return match[2].str();
            }
            return "";
        };

        metadata.name       = extractValue("name", deviceBlock);
        metadata.size       = extractValue("size", deviceBlock);
        metadata.model      = extractValue("model", deviceBlock);
        metadata.serial     = extractValue("serial", deviceBlock);
        metadata.uuid       = extractValue("uuid", deviceBlock);
        return metadata;
    }

    static std::string fingerprinting(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*)input.c_str(), input.size(), hash);
        std::string fingerprint;

        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", hash[i]);
            fingerprint += hex;
        }
        
        if (fingerprint.empty()) {
            Logger::log("[ERROR] Failed to generate fingerprint -> DriveFingerprinting::fingerprinting()", g_no_log);
            std::cerr << RED << "[ERROR] Failed to generate fingerprint\n" << RESET << "\n";
            return "";
        }
        return fingerprint;
    }


public:
    static void fingerprinting_main() {
        //std::string drive_name_fingerprinting = getAndValidateDriveName("Enter the Name of the drive you want to fingerprint");
        std::string drive_name_fingerprinting = listDrives(true);
        DriveMetadata metadata = getMetadata(drive_name_fingerprinting);

        Logger::log("[INFO] Retrieved metadata for drive: " + drive_name_fingerprinting + " -> DriveFingerprinting::fingerprinting_main()", g_no_log);

        std::string combined_metadatat =
            metadata.name + "|" +
            metadata.size + "|" +
            metadata.model + "|" +
            metadata.serial + "|" +
            metadata.uuid;
        std::string fingerprint = fingerprinting(combined_metadatat);

        Logger::log("[INFO] Generated fingerprint for drive: " + drive_name_fingerprinting + " -> DriveFingerprinting::fingerprinting_main()", g_no_log);

        std::cout << "A custom fingerprint was made for your drive\n";
        std::cout << BOLD << "Fingerprint:\n" << RESET;
        std::cout << "\r\033[K\n";
        std::cout << fingerprint << "\n";
    }
};


// ==================== Main Menu and Utilities ====================

void Info() {
    std::cout << "\n┌──────────" << BOLD << " Info " << RESET << "──────────\n";
    std::cout << "│ Welcome to Drive Manager — a program for Linux to view and operate your storage devices.\n"; 
    std::cout << "│ Warning! You should know the basics about drives so you don't lose any data.\n";
    std::cout << "│ If you find problems or have ideas, visit the GitHub page and open an issue.\n";
    std::cout << "│ Other info:\n";
    std::cout << "│ Version: " << VERSION << "\n";
    std::cout << "│ Github: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux\n";
    std::cout << "│ Author: Dogwalker-kryt\n";
    std::cout << "└───────────────────────────\n";
}

static void printUsage(const char* progname) {
    std::cout << "Usage: " << progname << " [options]\n";
    std::cout << BOLD << "Options:\n" << RESET;
    std::cout << "  --version, -v       Print program version and exit\n";
    std::cout << "  --help, -h          Show this help and exit\n";
    std::cout << "  -n, --dry-run       Do not perform destructive operations\n";
    std::cout << "  -C, --no-color      Disable colors (may affect the main menu)\n";
    std::cout << "  --no-log            Disables all logging in the current session\n";
    std::cout << "  --operation-name    Goes directly to a specific operation without menu\n";
    std::cout << "                      Available operations:\n";
    std::cout << "                        --list-drives\n";
    std::cout << "                        --format-drive\n";
    std::cout << "                        --encrypt-decrypt\n";
    std::cout << "                        --resize-drive\n";
    std::cout << "                        --check-drive-health\n";
    std::cout << "                        --analyze-disk-space\n";
    std::cout << "                        --overwrite-drive-data\n";
    std::cout << "                        --view-metadata\n";
    std::cout << "                        --info\n";
    std::cout << "                        --forensics\n";
    std::cout << "                        --disk-space-visualizer\n";
    std::cout << "                        --clone-drive\n";
    
}

void menuQues(bool& running) {   
    std::cout << BOLD <<"\nPress '1' for returning to the main menu, '2' to exit:\n" << RESET;
    int menuques;
    std::cin >> menuques;

    if (menuques == 1) {
        running = true;
    } else if (menuques == 2) {
        running = false;
    } else {
        std::cout << RED << "[ERROR] Wrong input\n" << RESET;
        running = true; 
    }
}

bool isRoot() {
    return (getuid() == 0);
}

void checkRoot() {
    if (!isRoot()) {
        std::cerr << "[ERROR] This Function requires root. Please run with 'sudo'.\n";
        Logger::log("[ERROR] Attempted to run without root privileges -> checkRoot()", g_no_log);
        exit(EXIT_FAILURE);
    }
}

void checkRootMetadata() {
    if (!isRoot()) {
        std::cerr << YELLOW << "[WARNING] Running without root may limit functionality. For full access, please run with 'sudo'.\n" << RESET;
        Logger::log("[WARNING] Running without root privileges -> checkRootMetadata()", g_no_log);
    }
}

enum MenuOptionsMain {
    EXITPROGRAM = 0, LISTDRIVES = 1, FORMATDRIVE = 2, ENCRYPTDECRYPTDRIVE = 3, RESIZEDRIVE = 4, 
    CHECKDRIVEHEALTH = 5, ANALYZEDISKSPACE = 6, OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8, VIEWINFO = 9,
    MOUNTUNMOUNT = 10, FORENSIC = 11, DISKSPACEVIRTULIZER = 12, LOGVIEW = 13, CLONEDRIVE = 14, CONFIG = 15, BENCHMAKR = 16, FINGERPRINT = 17
};

class QuickAccess {
public:
    static void list_drives() {
        listDrives(false);
    }

    static void foramt_drive() {
        checkRoot();
        formatDrive();
    }

    static void de_en_crypt() {
        checkRoot();
        DeEncrypting::main();
    }

    static void resize_drive() {
        checkRoot();
        resizeDrive();
    }

    static void check_drive_health() {
        checkRoot();
        checkDriveHealth();
    }

    static void analyze_disk_space() {
        analyDiskSpace();
    }

    static void overwrite_drive_data() {
        checkRoot();
        OverwriteDriveData();
    }

    static void view_metadata() {
        checkRootMetadata();
        MetadataReader::mainReader();
    }

    static void info() {
        Info();
    }

    static void Forensics() {
        bool no = false;
        checkRoot();
        ForensicAnalysis::mainForensic(no);
    }

    static void disk_space_visulizer() {
        checkRoot();
        DSV::DSVmain();
    }

    static void clone_drive() {
        checkRoot();
        Clone::mainClone();
    } 
};


// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    std::cout << "\033[?1049h";

    colorThemeHandler();

    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        if (a == "--dry-run" || a == "-n") {
            g_dry_run = true;
            continue;
        }

        if (a == "--no-color" || a == "--no_color" || a == "-C") {
            g_no_color = true;
            continue;
        }

        if ( a == "--no-log" || a == "--no_log") {
            g_no_log = true;
            continue;
        }

        if (a == "--help" || a == "-h") {
            printUsage(argv[0]);
            return 0;
        }

        if (a == "--version" || a == "-v") {
            std::cout << "DriveMgr CLI version: " << VERSION << "\n";
            return 0;
        }

        if (a == "--logs") {
            logViewer();
            return 0;
        }

        if (a == "--list-drives") {
            QuickAccess::list_drives();
            return 0;
        }

        if (a == "--format-drive") {
            QuickAccess::foramt_drive();
            return 0;
        }

        if (a == "--encrypt-decrypt") {
            QuickAccess::de_en_crypt();
            return 0;
        }

        if (a == "--resize-drive") {
            QuickAccess::resize_drive();
            return 0;
        }

        if (a == "--check-drive-health") {
            QuickAccess::check_drive_health();
            return 0;
        }

        if (a == "--analyze-disk-space") {
            QuickAccess::analyze_disk_space();
            return 0;
        }

        if (a == "--overwrite-drive-data") {
            QuickAccess::overwrite_drive_data();
            return 0;
        }

        if (a == "--view-metadata") {
            QuickAccess::view_metadata();
            return 0;
        }

        if (a == "--info") {
            QuickAccess::info();
            return 0;
        }

        if (a == "--forensics") {
            QuickAccess::Forensics();
            return 0;
        }

        if (a == "--disk-space-visualizer") {
            QuickAccess::disk_space_visulizer();
            return 0;
        }

        if (a == "--clone-drive") { 
            QuickAccess::clone_drive();
            return 0;
        }
    }
    
    // dry run helper
    CONFIG_VALUES cfg = configHandler();
    bool dry_run_mode = cfg.DRY_RUN_MODE;

    if (dry_run_mode == true) {
        g_dry_run = true;
    }

    bool flag_dry_run = false;
    bool flag_dry_run_set = false;

    // parse command line args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dry-run" || arg == "-n") {
            flag_dry_run = true;
            flag_dry_run_set = true;
        }
    }

    if (flag_dry_run_set) {
        g_dry_run = true;
    } 

    // TUI
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    bool running = true;
    while (running == true) {
        /* Original CLI menu (commented out so it's preserved for later):
        auto res = EXEC("clear"); std::string clear = res.output;
        std::cout << clear;
            std::cout << "┌─────────────────────────────────────────────────┐\n";
            std::cout << "│              DRIVE MANAGEMENT UTILITY           │\n";
            std::cout << "├─────────────────────────────────────────────────┤\n";
            std::cout << "│ 1.- List Drives                                 │\n";
            std::cout << "│ 2.  Format Drive                                │\n";
            std::cout << "│ 3.- Encrypt/Decrypt Drive (AES-256)             │\n";
            std::cout << "│ 4.  Resize Drive                                │\n";
            std::cout << "│ 5.- Check Drive Health                          │\n";
            std::cout << "│ 6.  Analyze Disk Space                          │\n";
            std::cout << "│ 7.- Overwrite Drive Data                        │\n";
            std::cout << "│ 8.  View Drive Metadata                         │\n";
            std::cout << "│ 9.- View Info/help                              │\n";
            std::cout << "│10.  Mount/Unmount/restore (ISO/Drives/USB)      │\n";
            std::cout << "│11.- Forensic Analysis (Beta)                    │\n";
            std::cout << "│12.  Disk Space Visualizer (Beta)                │\n";
            std::cout << "│13.- Log viewer                                  │\n";
            std::cout << "│14.  Clone a Drive                               │\n";
            std::cout << "│ 0.  Exit                                        │\n";
            std::cout << "└─────────────────────────────────────────────────┘\n";
            std::cout << "choose an option [0 - 12]:\n";
            int menuinput;
            std::cin >> menuinput;
        */

        // New cursor-driven menu for easier selection (arrow keys + Enter).
        // This only replaces the selection mechanism; operations still prompt for drive names.

        std::vector<std::pair<int, std::string>> menuItems = {
            {1, "List Drives"}, {2, "Format Drive"}, {3, "Encrypt/Decrypt Drive (AES-256)"},
            {4, "Resize Drive"}, {5, "Check Drive Health"}, {6, "Analyze Disk Space"},
            {7, "Overwrite Drive Data"}, {8, "View Drive Metadata"}, {9, "View Info/help"},
            {10, "Mount/Unmount/Restore (ISO/Drives/USB)"}, {11, "Forensic Analysis/Disk Image"},
            {12, "Disk Space Visualizer (Beta)"}, {13, "Log viewer"}, {14, "Clone a Drive"}, {15, "Config Editor"}, {16, "Benchmark"}, {17, "Fingerprint Drive"}, {0, "Exit"}
        };

        // enable raw mode for single-key reading
        // struct termios oldt, newt;
        // tcgetattr(STDIN_FILENO, &oldt);
        // newt = oldt;
        // newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        int selected = 0;
        while (true) {
            auto res = EXEC("clear"); std::string clear = res.output;
            std::cout << clear;
            std::cout << "Use Up/Down arrows and Enter to select an option.\n\n";
            std::cout << THEME_COLOR << "┌─────────────────────────────────────────────────┐\n" << RESET;
            std::cout << THEME_COLOR << "│" << RESET << BOLD << "              DRIVE MANAGEMENT UTILITY           " << RESET << THEME_COLOR << "│\n" << RESET;
            std::cout << THEME_COLOR << "├─────────────────────────────────────────────────┤\n" << RESET;
            for (size_t i = 0; i < menuItems.size(); ++i) {
                // Print left border
                std::cout << THEME_COLOR << "│ " << RESET;

                // Build inner content with fixed width
                std::ostringstream inner;
                inner << std::setw(2) << menuItems[i].first << ". " << std::left << std::setw(43) << menuItems[i].second;
                std::string innerStr = inner.str();

                if (menuItems[i].first == 0) {
                    innerStr = CYAN + innerStr + RESET;
                }

                // Apply inverse only to inner content
                if ((int)i == selected) std::cout << INVERSE;
                std::cout << innerStr;
                if ((int)i == selected) std::cout << RESET;

                // Print right border and newline
                std::cout << THEME_COLOR << " │\n" << RESET;




            }
            std::cout  << THEME_COLOR << "└─────────────────────────────────────────────────┘\n" << RESET;

            char c = 0;
            if (read(STDIN_FILENO, &c, 1) <= 0) continue;
            if (c == '\x1b') { // escape sequence
                char seq[2];
                if (read(STDIN_FILENO, &seq, 2) == 2) {
                    if (seq[1] == 'A') { // up
                        selected = (selected - 1 + (int)menuItems.size()) % (int)menuItems.size();
                    } else if (seq[1] == 'B') { // down
                        selected = (selected + 1) % (int)menuItems.size();
                    }
                }
            } else if (c == '\n' || c == '\r') {
                break; // selection made
            } else if (c == 'q' || c == 'Q') {
                // allow quick quit
                selected = (int)menuItems.size() - 1; // Exit item index
                break;
            }
        }

        // restore terminal
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        int menuinput = menuItems[selected].first;
        switch (static_cast<MenuOptionsMain>(menuinput)) {
            case LISTDRIVES: {
                listDrives(false);
                std::cout << BOLD << "\nPress '1' to return, '2' for advanced listing, or '3' to exit:\n" << RESET;
                int menuques2;
                std::cin >> menuques2;
                if (menuques2 == 1) continue;
                else if (menuques2 == 2) listpartisions();
                else if (menuques2 == 3) running = false;
                break;
            }

            case FORMATDRIVE: {
                checkRoot();
                formatDrive();
                menuQues(running);
                break;
            }

            case ENCRYPTDECRYPTDRIVE: {
                checkRoot();
                DeEncrypting::main();
                menuQues(running);
                break;
            }

            case RESIZEDRIVE: {
                checkRoot();
                resizeDrive();
                menuQues(running);
                break;
            }

            case CHECKDRIVEHEALTH:{
                checkRoot();
                checkDriveHealth();
                menuQues(running);
                break;
            }

            case ANALYZEDISKSPACE: {
                analyDiskSpace();
                menuQues(running);
                break;
            }

            case OVERWRITEDRIVEDATA: {
                checkRoot();
                std::cout << YELLOW << "[Warning]" << RESET << "This function will overwrite the entire data to zeros. Proceed? (y/n)\n";
                char zerodriveinput;
                std::cin >> zerodriveinput;
                if (zerodriveinput == 'y' || zerodriveinput == 'Y') OverwriteDriveData();
                else std::cout << "[Info] Operation cancelled\n";
                menuQues(running);
                break;
            }

            case VIEWMETADATA: {
                checkRootMetadata();
                MetadataReader::mainReader();
                menuQues(running);
                break;
            }

            case VIEWINFO: {
                Info();
                menuQues(running);
                break;
            }

            case MOUNTUNMOUNT: {
                checkRoot();
                MountUtility::mainMountUtil();
                menuQues(running);
                break;
            }

            case FORENSIC: {
                checkRoot();
                ForensicAnalysis::mainForensic(running);
                menuQues(running);
                break;
            }
            
            case DISKSPACEVIRTULIZER: {
                DSV::DSVmain();
                menuQues(running);
                break; 
            }

            case EXITPROGRAM: {
                running = false;
                break;
            }

            case LOGVIEW: {
                logViewer();
                menuQues(running);
                break;
            }

            case CLONEDRIVE: {
                checkRoot();
                Clone::mainClone();
                menuQues(running);
                break;
            }

            case CONFIG: {
                configEditor();
                menuQues(running);
                break;
            }
            
            case BENCHMAKR: {
                checkRoot();
                benchmarkMain();
                menuQues(running);
                break;
            }

            case FINGERPRINT: {
                checkRootMetadata();
                DriveFingerprinting::fingerprinting_main();
                menuQues(running);
                break;
            }

            default: {
                std::cerr << RED << "[Error] Invalid selection\n" << RESET;
                break;
            }
        }
    }
    std::cout << "\033[?1049l";
    return 0;
}
