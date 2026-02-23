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

// ! Warning this version is the experimental version of the program,
// This version has the latest and newest functions, but may contain bugs and errors
// Current version of this code is in the VERSION macro below and in the line bellow
// v0.9.19.19

// C++ libraries
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

// system includes
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <termios.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>

// openssl includes
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

// custom includes
#include "../include/DmgrLib.h"
#include "../include/debug.h"
#include "../include/exec_cmd.h"
#include "../include/dmgr_updater.h"

// │ ├ ┤ ┘ └ ┐ ┌ ─
// ==================== global variables and definitions ====================

// Version
#define VERSION std::string("v0.9.19.19")

// TUI
struct termios oldt; 
struct termios newt;

// flags
       bool g_dry_run   =  false;
       bool g_no_color  =  false;
       bool g_no_log    =  false;
static bool g_debug     =  false;

// color
static std::string THEME_COLOR      =   RESET;
static std::string SELECTION_COLOR  =   RESET;


// ==================== Side/Helper Functions ====================

static std::vector<std::string> g_last_drives;

/**
 * @brief Checks the filesystem of a given device and returns its status.
 * @param device The device path to check (e.g., "/dev/sda1").
 * @param fstype The filesystem type of the device (e.g., "ext4", "ntfs").
 */
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

// ==================== Main Logic Function and Classes ====================

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

    // TUI selection will stay here till i find a solution to put it in its own function

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
                if (i == selected) {
                    std::cout << SELECTION_COLOR << "> " << RESET;
                } else {
                    std::cout << "  ";
                }

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


// ==================== Partition Management ==================== 

class PartitionsUtils {
    public:
        // 1
        static bool resizePartition(const std::string& device, uint64_t newSizeMB) {
            try {
                std::string cmd = "parted --script " + device + " resizepart 1 " + std::to_string(newSizeMB) + "MB";
                                 
                auto res = EXEC(cmd);
                return res.success;

            } catch (const std::exception&) {
                return false;
            }
        }

        // 2
        static bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB) {
            try {
                std::string cmd = "parted --script " + device + " move " + std::to_string(partNum) + " " + std::to_string(startSectorMB) + "MB";
                                 
                auto res = EXEC_SUDO(cmd);
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

                std::string cmd = "echo 'type=" + newType + "' | sfdisk --part-type " + device + " " + std::to_string(partNum);
                                 
                auto res = EXEC(cmd); 
                std::string output = res.output;
                return output.find("error") == std::string::npos;

            } catch (const std::exception&) {
                return false;
            }
        }
};

void listpartisions() { 
    std::string drive_name = listDrives(true); 

    std::cout << "\nPartitions of drive " << drive_name << ":\n";

    std::string cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p " + drive_name; 
    auto res = EXEC_QUIET(cmd); 

    debug_msg("after EXEC(cmd) command", g_debug);

    std::istringstream iss(res.output);
    std::string line;

    std::cout << std::left 
              << std::setw(2)
              << std::setw(4) << "/"
              << std::setw(10) << "Name" 
              << std::setw(10) << "Size" 
              << std::setw(10) << "Type" 
              << std::setw(15) << "Mountpoint" 
              << std::setw(10) << "FSType" 
              << "\n";
    std::cout << std::string(63, '-') << "\n";

    std::vector<std::string> partitions;

    while (std::getline(iss, line)) {
        if (line.find("part") != std::string::npos) {
            std::istringstream lss(line);
            std::string part_name, part_size, part_type, part_mount, part_fstype;
            
            lss >> part_name >> part_size >> part_type;
            
            // Get rest of line for mountpoint and fstype
            std::string rest;
            std::getline(lss, rest);
            std::istringstream rss(rest);
            rss >> part_mount >> part_fstype;
            
            if (part_mount == "-") part_mount = "";
            if (part_fstype == "-") part_fstype = "";
            
            // Print formatted row
            std::cout << std::left
                      << std::setw(18) << part_name
                      << std::setw(10) << part_size
                      << std::setw(10) << part_type
                      << std::setw(15) << part_mount
                      << std::setw(10) << part_fstype
                      << "\n";
            
            partitions.push_back(part_name);
        }
    }

    if (partitions.empty()) {
        std::cout << RED << "[ERROR] No partitions found on this drive.\n" << RESET;
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
 
void analyzeDiskSpace() {
    std::string drive_name = listDrives(true); 

    std::cout  << CYAN << "\n------ Disk Information ------\n" << RESET;

    std::string disk_cmd = "lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + drive_name;
    auto res = EXEC(disk_cmd); std::string disk_output = res.output;

    std::istringstream iss(disk_output);

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

class FormatUtils {
private:
    static bool confirm_format(const std::string& drive, const std::string& label = "", const std::string& fs_type = "") {
        std::ostringstream msg;
        msg << "Are you sure you want to format: " << drive;
        if (!label.empty()) msg << " with label: " << label;
        if (!fs_type.empty()) msg << " and filesystem: " << fs_type;
        msg << "? (y/N)\n";
        
        std::cout << msg.str();
        std::string confirmation = "n";
        std::getline(std::cin, confirmation);
        
        if (confirmation != "y" && confirmation != "Y") {
            std::cerr << RED << "[INFO] " << RESET << "Formatting cancelled by user\n";
            Logger::log("Formatting cancelled for drive: " + drive, g_no_log);
            return false;
        }
        return true;
    }

public:
    static void format_drive(const std::string& drive_to_format, const std::string& label = "", const std::string& fs_type = "ext4") {
        if (!label.empty() && label.length() > 0) {
            if (label.length() > 16) {
                std::cerr << RED << "[ERROR] Label too long (max 16 chars)\n" << RESET;
                return;
            }
        }
        
        if (!confirm_format(drive_to_format, label, fs_type)) return;
        
        try {
            std::ostringstream cmd;
            cmd << "mkfs." << (fs_type.empty() ? "ext4" : fs_type);
            if (!label.empty()) cmd << " -L " << label;
            cmd << " " << drive_to_format;
            
            auto res = EXEC(cmd.str());
            
            if (!res.success) {
                std::cerr << RED << "[ERROR] Failed to format drive: " << drive_to_format << "\n" << RESET;
                Logger::log("[ERROR] Format failed: " + drive_to_format, g_no_log);
                return;
            }
            
            std::cout << res.output << "\n";
            std::cout << GREEN << "[INFO] Drive formatted successfully\n" << RESET;
            Logger::log("[INFO] Drive formatted: " + drive_to_format, g_no_log);
            
        } catch(const std::exception& e) {
            std::cerr << RED << "[ERROR] Exception: " << e.what() << "\n" << RESET;
            Logger::log("[ERROR] Format exception: " + std::string(e.what()), g_no_log);
        }
    }
    
    // Wrapper functions for backward compatibility
    static void formatDriveBasic(const std::string& drive) { format_drive(drive); }
    static void formatDriveWithLabel(const std::string& drive, const std::string& label) { format_drive(drive, label); }
    static void formatDriveWithLabelAndFS(const std::string& drive, const std::string& label, const std::string& fs) { format_drive(drive, label, fs); }
};


void formatDrive() {
    std::cout << "\n┌────── Choose an option for how to format ──────┐\n";
    std::cout << "├─────────────────────────────────────────────────┤\n";
    std::cout << "│ 1. Format drive                                 │\n";
    std::cout << "│ 2. Format drive with label                      │\n";
    std::cout << "│ 3. Format drive with label and filesystem       │\n";
    std::cout << "└─────────────────────────────────────────────────┘\n";
    std::cout << "INFO: Standard formatting will automaticlly use ext4 filesystem\n";
    int fdinput;
    std::cin >> fdinput;

    switch (fdinput) {
        case 1:
            {
                std::cout << "Choose a Drive to Format\n";
                std::string driveName = listDrives(true);

                FormatUtils::formatDriveBasic(driveName);
            }

            break;

        case 2:
            {
                std::cout << "Choose a Drive to Format with label\n";
                std::string driveName = listDrives(true);

                std::string label;
                std::cout << "Enter label: ";
                std::getline(std::cin, label);

                FormatUtils::formatDriveWithLabel(driveName, label);
            }

            break;

        case 3:
            {
                std::cout << "Choose a Drive to Format with label and filesystem type\n";
                std::string driveName = listDrives(true);

                std::string label;
                std::cout << "Enter label: ";
                std::getline(std::cin, label);

                std::string fsType;
                std::cout << "Enter filesystem type (e.g. ext4, ntfs, vfat): ";
                std::getline(std::cin, fsType);

                FormatUtils::formatDriveWithLabelAndFS(driveName, label, fsType);
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
    std::string driveHealth_name = listDrives(true);

    try {

        std::string health_cmd = "smartctl -H " + driveHealth_name;
        auto res = EXEC_SUDO(health_cmd); 

    } catch(const std::exception& e) {

        std::string error = e.what();
        Logger::log("[ERROR]" + error, g_no_log);
        dmgr_runtime_error("Failed to check drive health: " + error + " -> checkDriveHealth()");

    }

   return 0;
}


// ==================== Drive Resizing ====================

void resizeDrive() {
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
    private:
        static std::string createSecureKeyFile(const unsigned char* key) {
            std::string tmp_key_file = "/tmp/drivemgr_key_" + std::to_string(getpid());
            std::ofstream kf(tmp_key_file, std::ios::binary | std::ios::trunc);

            if (!kf) {
                std::cerr << RED << "[Error] Unable to create temporary key file\n" << RESET;
                Logger::log("[ERROR] Unable to create temporary key file", g_no_log);
                dmgr_runtime_error("Unable to create temporary key file -> createSecureKeyFile()");
            }

            kf.write(reinterpret_cast<const char*>(key), 32);
            chmod(tmp_key_file.c_str(), S_IRUSR | S_IWUSR);
            return tmp_key_file;
        }

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
                dmgr_runtime_error("[Error] Failed to generate random key/IV");
                Logger::log("[ERROR] Failed to generate random key/IV for encryption -> generateKeyAndIV()", g_no_log);
            }
        }

        static void encryptDrive(const std::string& driveName) {
            EncryptionInfo info;
            info.driveName = driveName;
            generateKeyAndIV(info.key, info.iv);
            saveEncryptionInfo(info);
            std::stringstream ss;

            std::string tmp_key_file = createSecureKeyFile(info.key);

            ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
               << "--key-file " << tmp_key_file << " open " << driveName
               << " encrypted_" << std::filesystem::path(driveName).filename().string();

            Logger::log("[INFO] Encrypting drive: " + driveName, g_no_log);

                auto res = EXEC_SUDO(ss.str());
                std::string output = res.output;

            // remove temp key file
            unlink(tmp_key_file.c_str());

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

            std::string tmp_key_file = createSecureKeyFile(info.key);

            std::stringstream ss;
            ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
               << "--key-file " << tmp_key_file << " open " << driveName << " decrypted_"
               << std::filesystem::path(driveName).filename().string();

            auto res = EXEC_SUDO(ss.str());
            std::string output = res.output;

            // remove temp key file
            unlink(tmp_key_file.c_str());

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
                dmgr_runtime_error("[Error] Failed to generate salt");
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
    static bool confirm_with_key(const std::string& operation) {
        std::string key = confirmationKeyGenerator();
        std::cout << YELLOW << operation << RESET << "\n";
        std::cout << "Confirmation key:\n" << key << "\n";
        std::cout << "Enter the key to proceed:\n";
        
        std::string input;
        std::cin >> input;
        
        if (input != key) {
            std::cerr << RED << "[Error] Invalid confirmation key\n" << RESET;
            Logger::log("[ERROR] Invalid confirmation key", g_no_log);
            return false;
        }
        return true;
    }

    static void encrypting() {
        std::string drive_name = listDrives(true);
        
        std::cout << YELLOW << "[Warning] Encrypt " << drive_name << "? (y/n)" << RESET << "\n";
        char confirm;
        std::cin >> confirm;
        if (confirm != 'y' && confirm != 'Y') {
            std::cout << "[Info] Cancelled\n";
            Logger::log("[INFO] Encryption cancelled", g_no_log);
            return;
        }
        
        if (!confirm_with_key("[Warning] Confirm encryption")) return;
        
        EncryptionInfo info;
        info.driveName = drive_name;
        
        if (!RAND_bytes(info.key, 32) || !RAND_bytes(info.iv, 16)) {
            std::cerr << RED << "[Error] Failed to generate encryption key\n" << RESET;
            return;
        }
        
        EnDecryptionUtils::loadEncryptionInfo(drive_name, info);
        
        std::cout << "[Input] Encrypted device name: ";
        std::string encrypted_name;
        std::cin >> encrypted_name;
        
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << drive_name << " " << encrypted_name;
        
        auto res = EXEC_SUDO(ss.str());
        
        if (!res.success) {
            std::cerr << RED << "[Error] Encryption failed\n" << RESET;
            Logger::log("[ERROR] Encryption failed", g_no_log);
        } else {
            std::cout << GREEN << "[Success] Drive encrypted as " << encrypted_name << RESET << "\n";
            std::cout << "[Info] Key saved in " << KEY_STORAGE_PATH << "\n";
        }
    }
    
    static void decrypting() {
        std::string drive_name = listDrives(true);
        
        std::cout << "[Warning] Decrypt " << drive_name << "? (y/n)\n";
        char confirm;
        std::cin >> confirm;
        if (confirm != 'y' && confirm != 'Y') {
            std::cout << "[Info] Cancelled\n";
            Logger::log("[INFO] Decryption cancelled", g_no_log);
            return;
        }
        
        if (!confirm_with_key("[Warning] Confirm decryption")) return;
        
        EncryptionInfo info;
        if (!EnDecryptionUtils::loadEncryptionInfo(drive_name, info)) {
            std::cerr << RED << "[Error] No encryption key found\n" << RESET;
            return;
        }
        
        std::cout << "Enter encrypted device name to decrypt: ";
        std::string encrypted_name;
        std::cin >> encrypted_name;
        
        std::stringstream ss;
        ss << "cryptsetup -v --cipher aes-cbc-essiv:sha256 --key-size 256 "
           << "--key-file <(echo -n '" << std::string((char*)info.key, 32) << "') "
           << "open " << drive_name << " " << encrypted_name << "_decrypted";
        
        auto res = EXEC_SUDO(ss.str());
        
        if (!res.success) {
            std::cerr << RED << "[Error] Decryption failed\n" << RESET;
            Logger::log("[ERROR] Decryption failed", g_no_log);
        } else {
            std::cout << GREEN << "[Success] Drive decrypted\n" << RESET;
        }
    }

public:
    static void main() {
        std::cout << "Encrypt (e) or Decrypt (d)?\n";
        char choice;
        std::cin >> choice;
        
        if (choice == 'e' || choice == 'E') {
            encrypting();
        } else if (choice == 'd' || choice == 'D') {
            decrypting();
        } else {
            std::cerr << RED << "[ERROR] Invalid input\n" << RESET;
        }
    }
};


// ==================== Drive Data Overwriting ====================
// Tried my best to make this as safe and readable and maintainable as possible. v0.9.12.92

void OverwriteDriveData() { 
    std::string drive = listDrives(true);
    try {
        std::cout << YELLOW << "[WARNING]" << RESET << " Are you sure you want to overwrite all data on " << BOLD << drive << RESET << "? This action cannot be undone! (y/n)\n";
        
        char confirm;
        std::cin >> confirm;

        if (confirm != 'y' && confirm != 'Y') {

            std::cout << BOLD << "[Overwriting aborted]" << RESET << " The Overwriting process of " << drive << " was interupted by user\n";
            Logger::log("[INFO] Overwriting process aborted by user for drive: " + drive, g_no_log);
            return;

        }

        std::cout << "\nTo be sure you want to overwrite the data on " << BOLD << drive << RESET << " you need to enter the following safety key\n";

        std::string conf_key = confirmationKeyGenerator();

        Logger::log("[INFO] Confirmation key generated for overwriting drive: " + drive, g_no_log);

        std::cout << "\n" << conf_key << "\n";

        std::cout << "\nEnter the confirmation key:\n";

        std::string user_input;
        std::cin >> user_input;

        if (user_input != conf_key) {

            std::cout << BOLD << "[INFO]" << RESET << " The confirmationkey was incorrect, the overwriting process has been interupted\n";
            Logger::log("[INFO] Incorrect confirmation key entered, overwriting process aborted for drive: " + drive, g_no_log);
            return;

        }

        std::cout << YELLOW << "\n[Process]" << RESET << " Proceeding with overwriting all data on: " << drive << "\n";
        std::cout << " \n";

        try {
            auto res_urandom = EXEC_SUDO("dd if=/dev/urandom of=" + drive + " bs=1M status=progress && sync"); 
            auto res_zero = EXEC_SUDO("dd if=/dev/zero of=" + drive + " bs=1M status=progress && sync"); 
            
            if (!res_urandom.success && !res_zero.success) {

                std::cout << RED << "[ERROR] Failed to overwrite the drive: " << drive << "\n" << RESET;
                Logger::log("[INFO] Overwriting failed to complete for drive: " + drive, g_no_log);
                return;

            } else if (!res_urandom.success || !res_zero.success) {

                std::cout << YELLOW << "[Warning]" << RESET << " One of the overwriting operations failed, but the drive may have been partially overwritten. Please check the output and try again if necessary.\n";
                Logger::log("[WARNING] One of the overwriting operations failed for drive: " + drive, g_no_log);
                return;
            } 

            std::cout << GREEN << "[Success]" << RESET << " Overwriting completed successfully for drive: " << drive << "\n";
            Logger::log("[INFO] Overwriting completed successfully for drive: " + drive, g_no_log);
            return;

        } catch (const std::exception& e) {

            Logger::log("[ERROR] Failed during overwriting process for drive: " + drive + " Reason: " + e.what(), g_no_log);
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

        auto res = EXEC_QUIET(cmd); 
        std::string json = res.output;

        size_t deviceStart = json.find("{", json.find("["));
        size_t childrenPos = json.find("\"children\"", deviceStart);

        std::string deviceBlock = json.substr(deviceStart, childrenPos - deviceStart);

        auto extractValue = [&](const std::string& key, const std::string& from) -> std::string {

            std::regex pattern("\"" + key + "\"\\s*:\\s*(null|\"(.*?)\")");
            std::smatch match;
            
            if (std::regex_search(from, match, pattern)) {

                if (match[1] == "null") {
                    return "";

                } else {
                    return match[2].str();
                }
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

    static std::string removeFirstLines(const std::string& text, int n = 3) {
        std::string out = text;
        for (int i = 0; i < n; i++) {
            size_t pos = out.find('\n');
            if (pos == std::string::npos)
                return out; // nothing left to remove
            out.erase(0, pos + 1);
        }
        return out;
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
            std::string smartCmd = "smartctl -i " + metadata.name;
            auto res = EXEC_QUIET_SUDO(smartCmd); 
            std::string smartOutput = removeFirstLines(res.output, 4);

            if (!smartOutput.empty()) {
                std::cout << smartOutput;

            } else {
                std::cout << "[ERROR] SMART data not available/intalled\n";

            }
        }   

        //std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";         
    } 
    
public:
    static void mainReader() {
        try{
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
// IsoFileMetadataChecker and IsoBurner refactored; v0.9.13.93

class MountUtility {
private:
    static bool IsoFileMetadataChecker(const std::string& iso_path) {
        namespace fs = std::filesystem;

        if (!fs::exists(iso_path) || !fs::is_regular_file(iso_path)) {
            Logger::log("[ERROR] ISO file does not exist: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;
        }

        constexpr std::streamoff iso_magic_offset = 32769; // 16 * 2048 + 1
        if (fs::file_size(iso_path) < iso_magic_offset + 5) {
            Logger::log("[ERROR] ISO file too small to contain valid metadata: " + iso_path, g_no_log);
            return false;
        }

        std::ifstream iso_file(iso_path, std::ios::binary);
        if (!iso_file) {
            Logger::log("[ERROR] Cannot open ISO file: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;
        }

        iso_file.seekg(iso_magic_offset);

        char buffer[6] = {};
        if (!iso_file.read(buffer, 5)) {
            Logger::log("[ERROR] Failed to read ISO metadata: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;
        }

        // ISO9660 magic signature
        if (std::string(buffer) != "CD001") {
            Logger::log("[ERROR] Invalid ISO signature in file: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;
        }

        return true;
    }

    static void BurnISOToStorageDevice() {
        try {
            std::string drive_name = listDrives(true);

            std::cout << "Enter the path to the ISO/IMG file you want to burn on " << drive_name << ":\n";

            std::string iso_path;
            std::getline(std::cin >> std::ws, iso_path);

            if (size_t pos = iso_path.find_first_of("-'&|<>;\""); pos != std::string::npos) {
                std::cerr << RED << "[ERROR] Invalid characters in drive name!: " << pos << RESET << "\n";
                Logger::log("[ERROR] Invalid characters in drive name\n", g_no_log);
                return;
            }

            if (!IsoFileMetadataChecker(iso_path)) {
                std::cout << RED << "[ERROR] The provided file is not a valid ISO image: "  << iso_path << RESET << "\n";
                Logger::log("[ERROR] Invalid ISO file: " + iso_path + " -> BurnISOToStorageDevice()", g_no_log);
                return;
            }

            std::cout << "Are you sure you want to burn " << iso_path << " to " << drive_name << "? (y/n)\n";

            char confirmation;
            std::cin >> confirmation;

            if (confirmation != 'y' && confirmation != 'Y') {
                std::cout << YELLOW << "[INFO] Operation cancelled\n" << RESET;
                Logger::log("[INFO] Burn operation cancelled by user -> BurnISOToStorageDevice()", g_no_log);
                return;
            }

            std::string confirmation_key = confirmationKeyGenerator();
            std::cout << "\nEnter the confirmation key to proceed:\n";
            std::cout << confirmation_key << "\n";
                        
            std::string user_key_input;
            std::cin >> user_key_input;

            if (user_key_input != confirmation_key) {
                std::cout << RED << "[ERROR] Incorrect confirmation key.\n" << RESET;
                Logger::log("[ERROR] Incorrect confirmation key -> BurnISOToStorageDevice()", g_no_log);
                return;
            }

            std::cout << "\n" << YELLOW << "[PROCESS] Burning ISO to device...\n" << RESET;

            auto unmount_res = EXEC_SUDO("umount " + drive_name + "* 2>/dev/null || true"); 
            auto res = EXEC_SUDO("dd if=" + iso_path + " of=" + drive_name + " bs=4M status=progress && sync"); 

            if (!res.success) {
                std::cout << RED << "[ERROR] Failed to burn ISO: " << res.output << RESET << "\n";
                Logger::log("[ERROR] dd burn failed for drive: " + drive_name + " -> BurnISOToStorageDevice()", g_no_log);
                return;
            }

            std::cout << GREEN << "[SUCCESS] Successfully burned " << iso_path << " to " << drive_name << "\n" << RESET;

            Logger::log("[INFO] Successfully burned ISO to drive: " + drive_name + " -> BurnISOToStorageDevice()", g_no_log);

        } catch (const std::exception& e) {

            std::cout << RED << "[ERROR] Burn operation failed: " << e.what() << RESET << "\n";
            Logger::log("[ERROR] BurnISOToStorageDevice() exception: " + std::string(e.what()), g_no_log);
        }
    }

    /**
     * @brief wrapps the orgirnal unmount() and mount() funcs to gether in one
     * @param mount_or_unmount type in mount, you will get the mount function, type in unmount you will get the unmount fukntion
     */
    static void choose_mount_unmount(const std::string mount_or_unmount) {
        std::cout << "Enter the drive you want to " << mount_or_unmount << "\n";
        std::string drive_name = listDrives(true);

        std::string cmd;

        if (mount_or_unmount == "mount") {
            std::string cmd = "mount " + drive_name + " /mnt/" + std::filesystem::path(drive_name).filename().string();

        } else if (mount_or_unmount == "unmount") {
            cmd = "umount " + drive_name;

        }  else {
            std::cerr << RED << "[ERROR] Invalid action: " << mount_or_unmount << "\n" << RESET;
            Logger::log("[ERROR] Invalid mount/unmount action: " + mount_or_unmount, g_no_log);
            return;
        }

        auto res = EXEC_SUDO(cmd);

        if (!res.success) {
            std::cerr << RED << "[ERROR] " << RESET << drive_name << " Failed to " << mount_or_unmount << "\n";
            Logger::log("[ERROR] Failed to " + mount_or_unmount + " drive: " + drive_name + " -> choose_mount_unmount()", g_no_log);
            return;
        }
    }

    static void Restore_USB_Drive() {
        std::string restore_device_name = listDrives(true);
        try {

            std::cout << "Are you sure you want to overwrite/clean the ISO/Disk_Image from: " << restore_device_name << " ? [y/n]\n";
            char confirm = 'n';
            std::cin >> confirm;

            if (std::tolower(confirm) != 'y') {
                std::cout << "Restore process aborted\n";
                return;
            }

            EXEC_QUIET_SUDO("umount " + restore_device_name + "* 2>/dev/null || true");
            
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

            if (!partition_path.empty() && std::isdigit(partition_path.back())) {
                partition_path += "p1"; 
            
            } else { 
                partition_path += "1";
            }

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

        std::cout << "\n┌────────────" << BOLD << " Mount menu " << RESET << "────────────┐\n";
        std::cout << "│1. Burn iso/img to storage device   │\n";
        std::cout << "│2. Mount storage device             │\n";
        std::cout << "│3. Unmount storage device           │\n";
        std::cout << "│4. Restore usb from iso             │\n";
        std::cout << "│0. Rreturn to main menu             │\n";
        std::cout << "└────────────────────────────────────┘\n";

        int menuinputmount;
        std::cin >> menuinputmount;

        switch (menuinputmount) {
            case Burniso: {
                BurnISOToStorageDevice();
                break;  
            }

            case MountDrive: {
                choose_mount_unmount("mount");
                break;
            }

            case UnmountDrive: {
                choose_mount_unmount("unmount");
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
    static void mainForensic() {
        enum ForensicMenuOptions {
            Info = 1, CreateDisktImage = 2, ScanDrive = 3, Exit = 0
        };

        std::cout << "\n┌─────── Forensic Analysis menu ────────┐\n";
        std::cout << "│ 1. Info of the Analysis tool           │\n";
        std::cout << "│ 2. Create a disk image of a drive      │\n";
        std::cout << "│ 3. recover of system, files,..         │\n";                                                                                                                                                                                                                                                                                                                                                                                                        
        std::cout << "│ 0. Exit                                │\n";                                                                                                                                                                                                                                                                                                                                                                                               
        std::cout << "└────────────────────────────────────────┘\n";
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

            case Exit: {
                break;
            }

            default: {
                std::cout << "[Error] Invalid selection\n";
                return;
            }
        }
    }
};


// ==================== Clone Drive Utility ====================
// last updated v0.18.15
// validate func and tiny refacotring of funcs

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

                    auto res = EXEC_SUDO("dd if=" + source + " of=" + target + " bs=5M status=progress && sync");

                    if (!res.success) {
                        Logger::log("[ERROR] Failed to clone drive from " + source + " to " + target + " -> Clone::CloneDrive()", g_no_log);
                        dmgr_runtime_error("Clone operation failed");
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

        static std::string validateTargetDriveName(std::string &target_drive) {
            std::string media = "/media/";
            std::string mnt = "/mnt/";
            std::string dev = "/dev/";

            if (target_drive.empty()) {
                dmgr_runtime_error("[ERROR] Target drive cannot be empty");
            }

            if (target_drive.find(media) != std::string::npos || target_drive.find(mnt) != std::string::npos || target_drive.find(dev) != std::string::npos) {
                return target_drive;

            } else {
                dmgr_runtime_error("Target drive string doesn't contain any string that marks it as a drive path");
            }
            
            return target_drive;
        }
        
    public:
        static void mainClone() {
            try {
                std::cout << "\nChoose a Source drive to clone the data from it:\n";
                std::string source_drive = listDrives(true);

                std::cout << "\nChoose a Target drive to clone the data on to it (dont choose the same drive):\n";
                std::cout << YELLOW << "[WARNING]" << RESET << " Make sure to choose the mount path of the target" << BOLD << " (e.g., /media/target_drive)\n" << RESET;
                std::string target_drive;
                std::getline(std::cin, target_drive);
                std::string val_target = validateTargetDriveName(target_drive);


                if (source_drive == target_drive) {

                    Logger::log("[ERROR] Source and target drives are the same -> Clone::mainClone()", g_no_log);
                    dmgr_runtime_error("[ERROR] Source and target drives cannot be the same!");
                    return;

                } else {

                    CloneDrive(source_drive, target_drive);
                    return;
                }

            } catch (std::exception& e) {

                std::cerr << RED << "[ERROR] " << e.what() << RESET << "\n";
                Logger::log("[ERROR] " + std::string(e.what()), g_no_log);
                return; 
            }
        }
};


// ==================== Log Viewer Utility ====================
// v0.15.97; Color for special events when viewing log file

void logViewer() {
    std::string path = filePathHandler("/.local/share/DriveMgr/data/log.dat");

    std::ifstream file(path);

    if (!file) {
        Logger::log("[ERROR] Unable to read log file at " + path, g_no_log);
        std::cerr << RED << "[ERROR] Unable to read log file at path: " << path << RESET << "\n";
        std::cout << "Please read the log file manually at: " << path << "\n";
        return;
    }

    std::cout << "\nLog file content:\n";

    std::string line;

    while (std::getline(file, line)) {

        if (line.find("[ERROR]") != std::string::npos) {
            std::cout << RED << line << RESET << "\n";

        } else if (line.find("[EXEC]") != std::string::npos) {
            std::cout << CYAN << line << RESET << "\n";

        } else if (line.find("[WARNING]") != std::string::npos) {
            std::cout << YELLOW << line << RESET << "\n";

        } else {

            std::cout << line << "\n";
        }
    }
}


// ==================== Configuration Editor Utility ====================

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
    
    std::string conf_file = filePathHandler("/.local/share/DriveMgr/data/config.conf");

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

/**
 * Helper function to check if a file exists at the given path.
 * @param path The file path to check.
 * @return true if the file exists, false otherwise.
 */
bool fileExists(const std::string& path) { struct stat buffer; return (stat(path.c_str(), &buffer) == 0); }

void configEditor() {
    extern struct termios oldt, newt;
    CONFIG_VALUES cfg = configHandler();
    
    std::cout << "┌─────" << BOLD << " config values " << RESET << "─────┐\n";
    std::cout << "│ UI mode: "          << cfg.UI_MODE               << "\n";
    std::cout << "│ Compile mode: "     << cfg.COMPILE_MODE          << "\n";
    std::cout << "│ Root mode: "        << cfg.ROOT_MODE             << "\n";
    std::cout << "│ Theme Color: "      << cfg.THEME_COLOR_MODE      << "\n";
    std::cout << "│ Selection Color: "  << cfg.SELECTION_COLOR_MODE  << "\n";
    std::cout << "└─────────────────────────┘\n";   
   
    std::cout << "\nDo you want to edit the config file? (y/n)\n";
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
// so the arrays must be synced
// std::string available_colores[6] = { "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN"};
// std::string color_codes[6] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};

std::array<std::string, 6> available_colores = {
    "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN"
};

std::array<std::string, 6> color_codes = {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN
};

void colorThemeHandler() {
    CONFIG_VALUES cfg = configHandler();
    std::string color_theme_name = cfg.THEME_COLOR_MODE;
    std::string selection_theme_name = cfg.SELECTION_COLOR_MODE;

    size_t count = sizeof(available_colores) / sizeof(available_colores[0]);

    THEME_COLOR = RESET;

    for (size_t i = 0; i < count; i++) {
        if (available_colores[i] == color_theme_name) {
            THEME_COLOR = color_codes[i];
            break;
        }
    }

    SELECTION_COLOR = RESET;

    for (size_t i = 0; i < count; i++) {
        if (available_colores[i] == selection_theme_name) {
            SELECTION_COLOR = color_codes[i];
            break;
        }
    }
}


// ==================== Drive Benchmark Utility ====================
// benchamarkSequentitalWrite last made v0.18.15

double benchmarkSequentialWrite(const std::string& path, size_t totalMB = 512) {
    const size_t blockSize = 4 * 1024 * 1024; // 4MB
    static char buffer[blockSize];
    memset(buffer, 'A', blockSize);

    std::string tempFile = path + "/dmgr_benchmark.tmp";

    int fd = open(tempFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return -1;

    auto start = std::chrono::high_resolution_clock::now();

    size_t totalBytes = totalMB * 1024ULL * 1024ULL;
    size_t written = 0;

    while (written < totalBytes) {
        size_t toWrite = std::min(blockSize, totalBytes - written);
        ssize_t ret = write(fd, buffer, toWrite);
        if (ret < 0) break;
        written += ret;
    }

    fsync(fd);
    close(fd);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::remove(tempFile.c_str());
    return totalMB / diff.count();
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

    // int pad_w = innerWidth - (2 + w_str2.length());
    // int pad_r = innerWidth - (2 + r_str2.length());
    // int pad_i = innerWidth - (2 + i_str2.length());

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
    std::string benchmark_dir_name;

    std::cout << "Make sure you have around 1 GB free space on the Drive\n";
    std::cout << "And dont kill the program during the Benchmark, this can cause the temp benchmark file to be not deleted!";
    std::cin >> benchmark_dir_name;

    double write_speed_seq = benchmarkSequentialWrite(benchmark_dir_name);
    double read_speed_seq = benchmarkSequentialRead(benchmark_dir_name);
    double iops_speed = benchmarkRandomRead(benchmark_dir_name);

    printBenchmarkSpeeds(write_speed_seq, read_speed_seq, iops_speed);
}


// ==================== Drive Fingerprinting Utility ====================
// v0.9.14.93; first func to recieve new debug_msg() for testing

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

        // test
        debug_msg("going to run lsblk cmd for metadata", g_debug);

        auto res = EXEC_QUIET(cmd);
        std::string json = res.output;

        size_t deviceStart = json.find("{", json.find("["));
        size_t childrenPos = json.find("\"children\"", deviceStart);
        
        if (deviceStart == std::string::npos || childrenPos == std::string::npos) {
            Logger::log("[WARN] Could not parse metadata JSON for drive: " + drive, g_no_log);
            // test 
            debug_msg("couldnt parse metadata JSON", g_debug);
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

        // test
        debug_msg("going to generate fingerpint based on metadata", g_debug);

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

        // test
        debug_msg(fingerprint, g_debug);
        return fingerprint;
    }


public:
    static void fingerprinting_main() {
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

        std::cout << BOLD << "\nFingerprint:\n" << RESET;
        std::cout << "\r\033[K\n";
        std::cout << fingerprint << "\n";
    }
};

// ==================== Main Menu and Utilities ====================

void Info() {
    std::cout << "\n┌──────────" << BOLD << " Info " << RESET << "──────────\n";
    std::cout << "│ Welcome to Linux Drive Manager (DMgr) — a program for Linux to view and operate your storage devices.\n"; 
    std::cout << "│ Warning! You should know the basics about drives so you don't lose any data.\n";
    std::cout << "│ If you find problems or have ideas, visit the GitHub page and open an issue.\n";
    std::cout << "│ " << BOLD << "Other info:\n" << RESET;
    std::cout << "│ Version: " << BOLD << VERSION << RESET << "\n";
    std::cout << "│ Github: " << BOLD << "https://github.com/Dogwalker-kryt/Linux-Drive-Manager\n" << RESET;
    std::cout << "│ Author: " << BOLD << "Dogwalker-kryt\n" << RESET;
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
    std::cout << "  --debug             Enables debug messages in current session\n";
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
    EXITPROGRAM = 0,        LISTDRIVES = 1,         FORMATDRIVE = 2,        ENCRYPTDECRYPTDRIVE = 3,    RESIZEDRIVE = 4, 
    CHECKDRIVEHEALTH = 5,   ANALYZEDISKSPACE = 6,   OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8,           VIEWINFO = 9,
    MOUNTUNMOUNT = 10,      FORENSIC = 11,          LOGVIEW = 12,           CLONEDRIVE = 13,            CONFIG = 14, 
    BENCHMAKR = 15,         FINGERPRINT = 16,       UPDATER = 17
};

class QuickAccess {
public:
    static void list_drives()           { listDrives(false); }

    static void format_drive()          { checkRoot(); formatDrive(); }

    static void de_en_crypt()           { checkRoot(); DeEncrypting::main(); }

    static void resize_drive()          { checkRoot(); resizeDrive(); }

    static void check_drive_health()    { checkRoot(); checkDriveHealth(); }

    static void analyze_disk_space()    { analyzeDiskSpace(); }

    static void overwrite_drive_data()  { checkRoot(); OverwriteDriveData(); }

    static void view_metadata()         { checkRootMetadata(); MetadataReader::mainReader(); }

    static void info()                  { Info(); }

    static void Forensics()             { checkRoot(); ForensicAnalysis::mainForensic(); }

    static void clone_drive()           { checkRoot(); Clone::mainClone(); } 
};


// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    std::cout << "\033[?1049h";

    colorThemeHandler();

    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);

        if (a == "--no-color" || a == "--no_color" || a == "-C")    { g_no_color = true; continue; }

        if ( a == "--no-log" || a == "--no_log")                    { g_no_log = true; continue; }

        if (a == "--help" || a == "-h")                             { printUsage(argv[0]); return 0; }

        if (a == "--version" || a == "-v")                          { std::cout << "DriveMgr CLI version: " << VERSION << "\n"; return 0; }

        if (a == "--debug")                                         { g_debug = true; continue; }

        if (a == "--logs")                                          { logViewer(); return 0; }

        if (a == "--list-drives")                                   { QuickAccess::list_drives(); return 0; }

        if (a == "--format-drive")                                  { QuickAccess::format_drive(); return 0; }

        if (a == "--encrypt-decrypt")                               { QuickAccess::de_en_crypt(); return 0; }

        if (a == "--resize-drive")                                  { QuickAccess::resize_drive(); return 0; }

        if (a == "--check-drive-health")                            { QuickAccess::check_drive_health(); return 0; }

        if (a == "--analyze-disk-space")                            { QuickAccess::analyze_disk_space(); return 0; }

        if (a == "--overwrite-drive-data")                          { QuickAccess::overwrite_drive_data(); return 0; }

        if (a == "--view-metadata")                                 { QuickAccess::view_metadata(); return 0; }

        if (a == "--info")                                          { QuickAccess::info(); return 0; }

        if (a == "--forensics")                                     { QuickAccess::Forensics(); return 0; }

        if (a == "--clone-drive")                                   { QuickAccess::clone_drive(); return 0; }

        if (a == "--fingerprint")                                   { DriveFingerprinting::fingerprinting_main(); return 0; }
    }
    
    // dry run helper
    CONFIG_VALUES cfg = configHandler();
    bool dry_run_mode = cfg.DRY_RUN_MODE;

    if (dry_run_mode == true) {
        g_dry_run = true;
    }

    bool flag_dry_run_set = false;

    // parse command line args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dry-run" || arg == "-n") {
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

        // New cursor-driven menu for easier selection (arrow keys + Enter).
        // This only replaces the selection mechanism; Some operation still call for manual input by typing.

        std::vector<std::pair<int, std::string>> menuItems = {
            {1, "List Drives"},                             {2, "Format Drive"},                                {3, "Encrypt/Decrypt Drive (AES-256)"},
            {4, "Resize Drive"},                            {5, "Check Drive Health"},                          {6, "Analyze Disk Space"},
            {7, "Overwrite Drive Data"},                    {8, "View Drive Metadata"},                         {9, "View Info/help"},
            {10, "Mount/Unmount/Restore (ISO/Drives/USB)"}, {11, "Forensic Analysis/Disk Image (experimental)"},
            {12, "Log viewer"},                             {13, "Clone a Drive"},                              {14, "Config Editor"}, 
            {15, "Benchmark (experimental)"},               {16, "Fingerprint Drive"},                          {17, "Updater"},
            {0, "Exit"}
        };

        // enable raw mode for single-key reading
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        int selected = 0;
        while (true) {
            auto res = EXEC("clear"); 
            std::cout << "Use Up/Down arrows and Enter to select an option.\n\n";
            std::cout << THEME_COLOR << "┌─────────────────────────────────────────────────┐\n" << RESET;
            std::cout << THEME_COLOR << "│" << RESET << BOLD << "              DRIVE MANAGEMENT UTILITY           " << RESET << THEME_COLOR << "│\n" << RESET;
            std::cout << THEME_COLOR << "├─────────────────────────────────────────────────┤\n" << RESET;
            for (size_t i = 0; i < menuItems.size(); ++i) {

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

            // 2. Format Drive
            case FORMATDRIVE:           { checkRoot(); formatDrive(); menuQues(running); break; }

            // 3. En-Decrypt Drive
            case ENCRYPTDECRYPTDRIVE:   { checkRoot(); DeEncrypting::main(); menuQues(running); break; }

            // 4. Resize Drive
            case RESIZEDRIVE:           { checkRoot(); resizeDrive(); menuQues(running); break; }

            // 5. Check Drive health
            case CHECKDRIVEHEALTH:      { checkRoot(); checkDriveHealth(); menuQues(running); break; }

            // 6. Analyze Drive Space
            case ANALYZEDISKSPACE:      { analyzeDiskSpace(); menuQues(running); break; }

            // 7. Overwrite Data
            case OVERWRITEDRIVEDATA:    { checkRoot(); OverwriteDriveData(); menuQues(running); break; }

            // 8. View Metadata
            case VIEWMETADATA:          { checkRootMetadata(); MetadataReader::mainReader(); menuQues(running); break; }

            // 9. View Info
            case VIEWINFO:              { Info(); menuQues(running); break; }

            // 10. Mount Unmount Drive
            case MOUNTUNMOUNT:          { checkRoot(); MountUtility::mainMountUtil(); menuQues(running); break; }

            // 11. Forensics
            case FORENSIC:              { checkRoot(); ForensicAnalysis::mainForensic(); menuQues(running); break; }

            // 12. Log viewer
            case LOGVIEW:               { logViewer(); menuQues(running); break; }

            // 13. Clone Drive
            case CLONEDRIVE:            { checkRoot(); Clone::mainClone(); menuQues(running); break; }

            // 14. Config edtior
            case CONFIG:                { configEditor(); menuQues(running); break; }
            
            // 15. Benchmark Drive
            case BENCHMAKR:             { checkRoot(); benchmarkMain(); menuQues(running); break; }

            // 16. Fingerprint Drive
            case FINGERPRINT:           { checkRootMetadata(); DriveFingerprinting::fingerprinting_main(); menuQues(running); break; }

            // 17. Updater
            case UPDATER:               { DmgrUpdater::updaterMain(VERSION); menuQues(running); break; }

            // 0. Exit
            case EXITPROGRAM:           { running = false; break; }

            default: {
                std::cerr << RED << "[Error] Invalid selection\n" << RESET;
                break;
            }
        }
    }
    
    std::cout << "\033[?1049l";
    return 0;
}
