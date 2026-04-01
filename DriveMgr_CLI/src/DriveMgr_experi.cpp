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
// v0.9.21.86_dev

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
#include <cstdint>
#include <ctime>
#include <random>
#include <unordered_map>
#include <optional>

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
#include "../include/LDM_updater.h"

// │ ├ ┤ ┘ └ ┐ ┌ ─
// ==================== global variables and definitions ====================

// === Version ===
#define VERSION std::string("v0.9.21.86_dev")


// === altTerminal Screen ===
/**
 * @brief Enters into a Alternate Terminal Screen
 */
#define NEWTERMINALSCREEN "\033[?1049h"

/**
 * @brief Leaves the Alternate Terminal Screen
 */
#define LEAVETERMINALSCREEN "\033[?1049l"


// === TUI ===
TerminosIO term;
struct termios oldt, newt;


// === color ===
static std::string g_THEME_COLOR      =   RESET;
static std::string g_SELECTION_COLOR  =   RESET;


// === flags ===
       bool g_dry_run   =  false;
       bool g_no_color  =  false;
       bool g_no_log    =  false;
static bool g_debug     =  false;


// === other global things ===
static std::vector<std::string> g_last_drives;
std::string g_selected_drive;


// ==================== Logic Function and Classes ====================

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

// ========== TUI drive selection/listing ==========

class ListDrivesUtil {
    private:
        struct Row {
            std::string device, size, type, mount, fstype, status;
        };

        inline static std::vector<ListDrivesUtil::Row> rows{};
        
        /**
         * @brief This is the Tui menu logic from the listDrive function finaly put in its own function
         * @param drives use the std::vector<std::string> where you stored your fetched drives
         * @param rows use the std::verctor<Row> rows, that is only in this class avilable
         */
        static std::string tuiForListDrives(const std::vector<std::string> &drives, std::vector<ListDrivesUtil::Row> &rows) {
            term.enableRawMode();

            int selected = 0;
            int total = drives.size();

            // Move cursor UP to the first drive row
            std::cout << "\033[" << total << "A";

            while (true) {
                std::cout << "\r"; // go to start of line
                for (int i = 0; i < total; i++) {

                    // Arrow indicator
                    if (i == selected) { std::cout << g_SELECTION_COLOR << "> " << RESET; }
                    else { std::cout << "  "; }

                    // Highlight row
                    if (i == selected) std::cout << g_SELECTION_COLOR;

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

            term.restoreTerminal();

            auto tui_selected_drive = drives[selected];
            return tui_selected_drive;
        }

    public:
        /**
         * @brief Prints lsblk output to the terminal. TUI input can be turned on
         * @param input_mode if 'true' then the TUI selection enables and returns the selected drive when pressed enter
         * @returns selected drive name as string. TUI must be enabled for this to happen
         */
        static std::string listDrives(bool input_mode) {
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
                ERR(ErrorCode::DeviceNotFound, "No drives found");
                Logger::error("No drives found", g_no_log);
                return "";
            }

            if (input_mode != true) {
                return "";
            }

            g_selected_drive = tuiForListDrives(drives, rows);
            return g_selected_drive;
        }   
};

// ========== Partition Management ========== 

class PartitionsUtils {
    private:
        // 1
        static bool resizePartition(const std::string& device, uint64_t newSizeMB) {
            try {

                std::string cmd = "parted --script " + device + " resizepart 1 " + std::to_string(newSizeMB) + "MB";
                                 
                auto res = EXEC(cmd);
                return res.success;

            } catch (const std::exception&) {

                ERR(ErrorCode::ProcessFailure, "Failed to resize partition");
                Logger::error("Failed to resize partition -> PartitionUtils::resizePartition()", g_no_log);
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

                ERR(ErrorCode::ProcessFailure, "Failed to move partition");
                Logger::error("Failed to move partition -> PartitionUtils::movePartition()", g_no_log);
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

                ERR(ErrorCode::ProcessFailure, "Failed to change partition type");
                Logger::error("Failed to change partition type -> PartitionUtils::changePartitionType()", g_no_log);
                return false;

            }
        }

    public:
        static void case1ResizePartition(const std::vector<std::string> &partitions) {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";
            int partNum;
            std::cin >> partNum;

            if (!std::cin) {

                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                ERR(ErrorCode::InvalidInput, "Expected input is a integer");
                Logger::error("Expected input is a integer", g_no_log);

            }

            if (partNum < 1 || partNum > (int)partitions.size()) {

                ERR(ErrorCode::OutOfRange, "Invalid partition number selected");
                return;

            }

            std::cout << "Enter new size in MB: ";
            uint64_t newSize;
            std::cin >> newSize;

            if (!std::cin) {

                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                ERR(ErrorCode::InvalidInput, "Expected a positive numeric input to assign to a uint64_t integer");
                Logger::error("You cannot assign a number <=0 to a uint64 interger", g_no_log);

            }

            if (newSize == 0) {

                ERR(ErrorCode::OutOfRange, "newSize cannot be equal to 0; Expected a number greater then 0 for uint64_t integer");
                Logger::error("newSize cannot be equal to 0 -> Partition Utils", g_no_log);
                return;

            }

            askForConfirmation("[Warning] Resizing partitions can lead to data loss.\nAre you sure? ");

            if (PartitionsUtils::resizePartition(partitions[partNum-1], newSize)) {

                std::cout << "Partition resized successfully!\n";

            } else {

                ERR(ErrorCode::ProcessFailure, "Failed to resize partition");
                Logger::error("Failed to resize partition", g_no_log);

            }
        }

        static void case2MovePartition(const std::vector<std::string> &partitions) {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";

            int partNum;
            std::cin >> partNum;

            if (!std::cin) {

                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                ERR(ErrorCode::InvalidInput, "Expected input is a integer");
                Logger::error("Expected input is a integer", g_no_log);

            }

            if (partNum < 1 || partNum > (int)partitions.size()) {

                ERR(ErrorCode::OutOfRange, "Invalid partition number selected");
                return;

            }

            std::cout << "Enter new start position in MB: ";
            uint64_t startPos;
            std::cin >> startPos;

            if (!std::cin) {

                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                ERR(ErrorCode::InvalidInput, "Expected a positive numeric input to assign to a uint64_t integer");
                Logger::error("You cannot assign a number <=0 to a uint64 interger", g_no_log);

            }

            if (startPos == 0) {

                ERR(ErrorCode::OutOfRange, "startPos cannot be equal to 0; Expected a number greater then 0 for uint64_t integer");
                Logger::error("startPos cannot be equal to 0 -> Partition Utils", g_no_log);
                return;

            }

            askForConfirmation("[Warning] Moving partitions can lead to data loss.\nAre you sure? ");

            if (movePartition(partitions[partNum-1], partNum, startPos)) {

                std::cout << "Partition moved successfully!\n";

            } else {

                ERR(ErrorCode::ProcessFailure, "Failed to move partition -> movePartition()");
                Logger::error("Failed to move partition", g_no_log);

            }
        }

        static void case3ChangePartitionType(const std::vector<std::string> &partitions, const std::string &drive_name) {
            std::cout << "Enter partition number (1-" << partitions.size() << "): ";

            int partNum;
            std::cin >> partNum;

            if (partNum < 1 || partNum > (int)partitions.size()) {

                ERR(ErrorCode::OutOfRange, "Invalid partition number selected");
                return;

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

            auto typeNum = validateIntInput(1, 4);
            std::string newType;

            if (!std::cin) {

                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                ERR(ErrorCode::InvalidInput, "Expected input is a integer");
                Logger::error("Expected input is a integer", g_no_log);

            }

            switch (*typeNum) {
                case 1: newType = "83"; break;
                case 2: newType = "7"; break;
                case 3: newType = "b"; break;
                case 4: newType = "82"; break;
                default:
                    ERR(ErrorCode::OutOfRange, "Invalid partition type selected");
                    break;
            }

            if (!newType.empty()) {

                askForConfirmation("[Warning] Changing partition type can make data inaccessible.\nAre you sure? ");

                if (changePartitionType(drive_name, partNum, newType)) {

                    std::cout << "Partition type changed successfully!\n";

                } else {

                    ERR(ErrorCode::ProcessFailure, "Failed to change partition type -> changePartitionType()");
                    Logger::error("Failed to change partition type", g_no_log);

                }
            }
        }
};

void listpartisions() { 
    std::string drive_name = ListDrivesUtil::listDrives(true); 

    std::cout << "\nPartitions of drive " << drive_name << ":\n";

    std::string cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p " + drive_name; 
    auto res = EXEC_QUIET(cmd); 

    if (!res.success) {

        ERR(ErrorCode::ProcessFailure, "lsblk failed");
        Logger::error("lsblk failed", g_no_log);
    }

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
        ERR(ErrorCode::DeviceNotFound, "No partitions found on this drive");
    }

    std::cout << "\n┌─ Partition Management Options ─┐\n";
    std::cout << "├────────────────────────────────┤\n";
    std::cout << "│ 1. Resize partition            │\n";
    std::cout << "│ 2. Move partition              │\n";
    std::cout << "│ 3. Change partition type       │\n";
    std::cout << "│ 4. Return to main menu         │\n";
    std::cout << "└────────────────────────────────┘\n";
    std::cout << "Enter your choice: ";

    auto choice = validateIntInput(1, 4);
    if (!choice.has_value()) return;

    switch (*choice) {
        case 1: {
            PartitionsUtils::case1ResizePartition(partitions);
            break;
        }

        case 2: {
            PartitionsUtils::case2MovePartition(partitions);
            break;
        }

        case 3: {
            PartitionsUtils::case3ChangePartitionType(partitions, drive_name);
            break;
        }

        case 4:
            return;
            
        default:
            ERR(ErrorCode::OutOfRange, "Invalid option selected in partition menu");
    }
}

// ========== Disk Space Analysis ==========··−·
 
void analyzeDiskSpace() {
    std::string drive_name = ListDrivesUtil::listDrives(true); 

    std::cout  << CYAN << "\n------ Disk Information ------\n" << RESET;

    std::string disk_cmd = "lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + drive_name;
    auto disk_cmd_res = EXEC(disk_cmd); 

    if (!disk_cmd_res.success || disk_cmd_res.output.empty()) {

        ERR(ErrorCode::ProcessFailure, "lsblk failed");
        return;

    }

    std::istringstream iss(disk_cmd_res.output);

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
        ERR(ErrorCode::DeviceNotFound, "No Disk found");
        return;

    } 

    if (!mount_point.empty() && mount_point != "-") {

        std::string df_cmd = "df -h '" + mount_point + "' | tail -1";
        auto df_res = EXEC(df_cmd); 
        std::string df_out = df_res.output;

        std::istringstream dfiss(df_out);

        std::string filesystem, df_size, used, avail, usep, mnt;
        dfiss >> filesystem >> df_size >> used >> avail >> usep >> mnt;

        std::cout << "Used:        " << used << "\n";
        std::cout << "Available:   " << avail << "\n";
        std::cout << "Used %:      " << usep << "\n";

    } else {

        std::cout << "No mountpoint, cannot show used/free space.\n";

    }
    
    std::cout << CYAN << "------------------------------\n" << RESET;
}


// ========== Drive Formatting ==========

class FormatUtils {
private:
    static bool confirm_format(const std::string& drive, const std::string& label = "", const std::string& fs_type = "") {
        std::ostringstream msg;

        msg << "Are you sure you want to format: " << drive;

        if (!label.empty()) msg << " with label: " << label;

        if (!fs_type.empty()) msg << " and filesystem: " << fs_type;

        msg << "? (y/N)\n";
        
        std::cout << msg.str();

        auto confirmation = validateCharInput({'y', 'n'});
        if (!confirmation.has_value()) return false;
        
        if (confirmation != 'y') {

            std::cout << RED << "[INFO] " << RESET << "Formatting cancelled by user\n";
            Logger::info("Formatting cancelled for drive: " + drive, g_no_log);
            return false;

        }

        return true;
    }

public:
    static void format_drive(const std::string& drive_to_format, const std::string& label = "", const std::string& fs_type = "ext4") {
        if (!label.empty()) {

            if (label.length() > 16) {

                ERR(ErrorCode::OutOfRange, "Label too long (max 16 chars) -> format_drive()");
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

                ERR(ErrorCode::ProcessFailure, "Failed to format drive: " + drive_to_format + " -> format_drive()");
                Logger::error("Format failed: " + drive_to_format, g_no_log);
                return;

            }
            
            std::cout << res.output << "\n";
            std::cout << GREEN << "[INFO] Drive formatted successfully\n" << RESET;

            Logger::info("Drive formatted: " + drive_to_format, g_no_log);
            
        } catch(const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Exception during formatting: " + std::string(e.what()) + " -> format_drive()");
            Logger::error("Format exception: " + std::string(e.what()), g_no_log);
            return;

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

    auto fdinput = validateIntInput(1, 3);
    if (!fdinput.has_value()) return;

    switch (*fdinput) {
        case 1:
            {
                std::cout << "Choose a Drive to Format\n";
                const std::string driveName = ListDrivesUtil::listDrives(true);

                FormatUtils::formatDriveBasic(driveName);
            }

            break;

        case 2:
            {
                std::cout << "Choose a Drive to Format with label\n";
                const std::string driveName = ListDrivesUtil::listDrives(true);

                std::string label;
                std::cout << "Enter label: ";
                std::getline(std::cin, label);

                FormatUtils::formatDriveWithLabel(driveName, label);
            }

            break;

        case 3:
            {
                std::cout << "Choose a Drive to Format with label and filesystem type\n";
                const std::string driveName = ListDrivesUtil::listDrives(true);

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

            ERR(ErrorCode::OutOfRange, "Invalid formatting option selected -> formatDrive()");
            return;

        }
    }
}

// ========== Drive Health Check ==========

int checkDriveHealth() {
    const std::string driveHealth_name = ListDrivesUtil::listDrives(true);

    try {

        std::string health_cmd = "smartctl -H " + driveHealth_name;
        auto res = EXEC_QUIET_SUDO(health_cmd);
        std::string health_output = removeFirstLines(res.output, 3); 
        std::cout << health_output;

    } catch(const std::exception& e) {

        std::string error = e.what();
        Logger::error(error, g_no_log);
        ERR(ErrorCode::ProcessFailure, e.what());

    }

   return 0;
}

// ========== Drive Resizing ==========

void resizeDrive() {
    const std::string driveName = ListDrivesUtil::listDrives(true);

    std::cout << "Enter new size in GB for drive " << driveName << ":\n";
    uint new_size;
    std::cin >> new_size;

    if (!std::cin) {

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        ERR(ErrorCode::InvalidInput, "Expected input is a uint integer");
        Logger::error("Expected input is a uint", g_no_log);
        return;

    }

    if (new_size == 0) {
        ERR(ErrorCode::OutOfRange, "new_size cannot be equal to 0; Expected a number greater then 0 for uint integer");
        return;
    }

    std::cout << "Resizing drive " << driveName << " to " << new_size << " GB...\n";

    try {

        std::string resize_cmd = "sudo parted --script " + driveName +  " resizepart 1 " + std::to_string(new_size) + "GB";
        auto res = EXEC(resize_cmd); std::string resize_output = res.output;

        std::cout << resize_output << "\n";

        if (resize_output.find("error") != std::string::npos) {

            ERR(ErrorCode::ProcessFailure, "Failed to resize drive: " + driveName);
            Logger::error("Failed to resize drive: " + driveName + " -> resizeDrive()", g_no_log);

        } else {

            std::cout << GREEN << "Drive resized successfully\n" << RESET;
            Logger::success("Drive resized successfully: " + driveName + " -> resizeDrive()", g_no_log);

        }

    } catch (const std::exception& e) {

        ERR(ErrorCode::ProcessFailure, "Exception during resize: " + std::string(e.what()) + " -> resizeDrive()");
        Logger::error("Exception during resize: " + std::string(e.what()) + " -> resizeDrive()", g_no_log);

    }
}

// ========== Drive Encryption ========== 
// new USB only encryption/decryption impl

class USBEnDeCryptionUtils {
private:
    enum class Metadata {
        TYPE,
        VENDOR,
        TRAN
    };

    struct MetadataHash {
        std::size_t operator()(Metadata m) const noexcept {
            return static_cast<std::size_t>(m);
        }
    };

    static std::optional<std::string> isValidDrive(const std::string &drive_name) {
        std::string cmd = "lsblk -o TYPE,VENDOR,TRAN -P -p " + drive_name; 
        auto res = EXEC_QUIET(cmd);

        if (!res.success || res.output.empty()) {

            ERR(ErrorCode::ProcessFailure, "lsblk failed to succed");
            Logger::error("lsblk failed to succed", g_no_log);
            return std::nullopt;

        }

        std::unordered_map<Metadata, std::string, MetadataHash> meta;

        auto extract = [&](const std::string& key) -> std::string {
            std::string search = key + "=\"";
            size_t start = res.output.find(search);
            if (start == std::string::npos) return "N/A";

            start += search.length();
            size_t end = res.output.find("\"", start);
            if (end == std::string::npos) return "N/A";

            return res.output.substr(start, end - start);
        };

        meta[Metadata::TYPE] = extract("TYPE");
        meta[Metadata::VENDOR] = extract("VENDOR");
        std::string tran = meta[Metadata::TRAN] = extract("TRAN");

        std::transform(tran.begin(), tran.end(), tran.begin(), ::tolower);

        if (meta[Metadata::TYPE] != "disk") {

            ERR(ErrorCode::InvalidDevice, "Drive is not a Disk " + drive_name);
            Logger::error("Drive is not a disk " + drive_name + "; -> isValidDrive USBEncryption", g_no_log);
            return std::nullopt;

        }

        if (meta[Metadata::VENDOR] == "N/A" || meta[Metadata::VENDOR] == "ATA") {

            ERR(ErrorCode::InvalidDevice, "Drive is an internal disk " + drive_name + "; Expected USB Drive");
            Logger::error("Drive is an internal Disk " + drive_name + "; -> isValidDrive USBEncryption", g_no_log);
            return std::nullopt;
                        
        }

        if (tran != "usb") {

            ERR(ErrorCode::InvalidDevice, "Drive is not an USB Device " + drive_name + "; Expected USB Drive");
            Logger::error("Drive is an internal Disk " + drive_name + "; -> isValidDrive USBEncryption", g_no_log);
            return std::nullopt;
                        
        }

        return drive_name;
    }

    static bool confirmationKeyInput() {
        std::cout << "\nTo proceed with anything you need to retype the following confirmation key:\n";

        std::string confirmation_key = confirmationKeyGenerator();
        std::cout << "\n" << confirmation_key << "\n";

        std::cout << "\nretype the key:\n";

        std::string user_retyped_key;
        std::getline(std::cin, user_retyped_key);

        if (user_retyped_key != confirmation_key) {

            std::cout << YELLOW << "[INFO] " << RESET << "The key you retyped doesnt match the original key\n" << "Process Aborted due to invalid input\n";
            Logger::info("The retyped key doesnt match the original key; Process Aborted due to invalid input", g_no_log);

            std::cout << "Do you want to retry? (y/N)\n";

            auto confirm_if_retry = validateCharInput({'y', 'n'});
            if (!confirm_if_retry.has_value()) return false;

            if (confirm_if_retry == 'n') {
                std::cout << YELLOW << "[INFO] " << RESET << "User aborted retry\n";
                Logger::info("Key retry was aborted by the user", g_no_log);
                return false;
            }

            std::string confirm_key2 = confirmationKeyGenerator();

            std::cout << "\n[last chance] Retype the following confirmation key:\n";
            std::cout << "\n" << confirm_key2 << "\n";

            std::string confirm_key2_input;
            std::getline(std::cin, confirm_key2_input);

            if (confirm_key2_input != confirm_key2) {
                std::cout << YELLOW << "[INFO] " << RESET
                        << "The key you retyped doesnt match the original key\n"
                        << "Process Aborted due to invalid input\n";
                Logger::info("The retyped key doesnt match the original key; Process Aborted due to invalid input", g_no_log);
                return false;
            }

            return true;
        }

        return true;
    }

    static void encryptUSBDrive(const std::string &drive_name) {
        //passphrases
        std::cout << BOLD << "\n[Encryption of " << drive_name << "]" << RESET << "\n" ;
        std::string passphrase, passphrase_retype;
        
        std::cout << RED << "\n[WARNING] " << RESET << "You should save or remember the passphrase!\n The DMgr will NOT! save it\n";
        std::cout << "\nEnter a Passphrase for the encrypted USB\n";
        std::getline(std::cin, passphrase);

        std::cout << "\nRetype your Passphrase you just entered:\n";
        std::getline(std::cin, passphrase_retype);
    
        if (passphrase.empty() || passphrase_retype.empty()) {

            ERR(ErrorCode::InvalidInput, "The passphrase you entered is emtpy; Expecting non empty string");
            Logger::error("The passphrase you entered is emtpy", g_no_log);
            return;

        }

        if (passphrase != passphrase_retype) {

            ERR(ErrorCode::InvalidInput, "The passphrases you entered doesnt match; Expecting similar passphrase input");
            Logger::error("The passphrases you entered doesnt match: p1:'" + passphrase + "' p2:'" + passphrase_retype + "'", g_no_log);
            return;

        }
    
        // pass passphrases to crypsetup
        std::string cryptsetup_cmd = "echo \"" + passphrase + "\" | cryptsetup luksFormat " + drive_name + " --key-file=- -q";
        auto cryptsetup_res = EXEC_SUDO(cryptsetup_cmd);
    
        if (!cryptsetup_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to execute cryptsetup on: " + drive_name + " with passphrase: " +  passphrase);
            Logger::error("Failed to execute cryptsetup on: " + drive_name, g_no_log);
            return;

        }

        // open encrypted device
        std::cout << "[INFO] open encrypted device...\n";
        std::string mapper_name = "enc_usb";
        std::string mapper_path = "/dev/mapper/" + mapper_name;

        std::string cryptsetup_open_cmd = "echo \"" + passphrase + "\" | cryptsetup open " + drive_name + " " + mapper_name + " --key-file=-";
        auto cryptsetup_open_res = EXEC_SUDO(cryptsetup_open_cmd);

        if (!cryptsetup_open_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to open drive crypsetup on: " + drive_name);
            Logger::error("Failed to open drive with cryptsetup on: " + drive_name, g_no_log);
            return;

        }       

        // When you need a diffrent FS then change it here
        std::string mkfs_ext4_cmd = "mkfs.ext4 " + mapper_path;
        auto mkfs_res = EXEC_SUDO(mkfs_ext4_cmd);

        if (!mkfs_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to make FS on " + mapper_name);
            Logger::error("Failed to make FS on " + mapper_name, g_no_log);
            return;            

        }

        // mount encrypted device
        std::cout << "[INFO] mounting encrypted device...\n";
        std::string mount_cmd = "mount " + mapper_path + " /media/" + mapper_name;
        auto mk_mountpoint_res = EXEC_SUDO("mkdir -p /media/" + mapper_name);
        auto mount_res = EXEC_SUDO(mount_cmd);

        if (!mk_mountpoint_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to create mountpoint");
            Logger::error("Failed to create mountpoint", g_no_log);
            return;     

        }

        if (!mount_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to mount " + mapper_name);
            Logger::error("Failed to mount " + mapper_name, g_no_log);
            return;            

        }    

        // close 
        std::cout << "[INFO] closing encrypted device...\n";
        auto unmount_cryptsetup_res = EXEC_SUDO("umount /media/" + mapper_name);

        if (!unmount_cryptsetup_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to unmount /media/" + mapper_name);
            Logger::error("Failed to unmount /media/" + mapper_name, g_no_log);
            return;

        }

        std::string close_cryptsetup_cmd = "cryptsetup close " + mapper_name;
        auto close_cryptsetup_res = EXEC_SUDO(close_cryptsetup_cmd);

        if (!close_cryptsetup_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to close " + mapper_name);
            Logger::error("Failed to close " + mapper_name, g_no_log);
            return;            

        }

        std::fill(passphrase.begin(), passphrase.end(), '\0');
        std::fill(passphrase_retype.begin(), passphrase_retype.end(), '\0');

        std::cout << GREEN << "\n[SUCCESS] " << RESET << "Encryption completed successfully\n";
        Logger::success("Encryption completed successfully", g_no_log);
        // TODO: maby custom listdrives func for printing only usb's; make that passphrse dont leak into shell
    }

    static void decryptUSBDrive(const std::string &drive_name) {
        std::cout << BOLD << "\n[Decryption / Unlock of " << drive_name << "]" << RESET << "\n";

        std::string passphrase;

        std::cout << RED << "\n[WARNING] " << RESET << "You must enter the correct passphrase to unlock this encrypted USB.\n";

        std::cout << "\nEnter the Passphrase:\n";
        std::getline(std::cin, passphrase);

        if (passphrase.empty()) {

            ERR(ErrorCode::InvalidInput, "Passphrase empty; expected non-empty string");
            Logger::error("Passphrase empty", g_no_log);
            return;

        }

        // mapper name
        std::string mapper_name = "enc_usb";
        std::string mapper_path = "/dev/mapper/" + mapper_name;

        // cryptsetup open
        std::string cryptsetup_open_cmd = "echo \"" + passphrase + "\" | cryptsetup open " + drive_name + " " + mapper_name + " --key-file=-";

        auto cryptsetup_open_res = EXEC_SUDO(cryptsetup_open_cmd);

        if (!cryptsetup_open_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to open encrypted device with cryptsetup");
            Logger::error("Failed to open encrypted device with cryptsetup", g_no_log);
            return;

        }

        // mount
        auto mk_mountpoint_res = EXEC_SUDO("mkdir -p /media/" + mapper_name);

        if (!mk_mountpoint_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to create mountpoint");
            Logger::error("Failed to create mountpoint", g_no_log);
            return;

        }

        std::string mount_cmd = "mount " + mapper_path + " /media/" + mapper_name;
        auto mount_res = EXEC_SUDO(mount_cmd);

        if (!mount_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to mount decrypted device");
            Logger::error("Failed to mount decrypted device", g_no_log);
            return;

        }

        std::cout << GREEN << "\n[SUCCESS] " << RESET  << "USB successfully unlocked and mounted at /media/" << mapper_name << "\n";
        Logger::success("USB successfully unlocked and mounted", g_no_log);

        std::cout << YELLOW << "[INFO] " << RESET << "Press ENTER when you are done using the USB to unmount and lock it again.\n";
        std::cin.get();

        // unmount
        auto unmount_res = EXEC_SUDO("umount /media/" + mapper_name);

        if (!unmount_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to unmount decrypted device");
            Logger::error("Failed to unmount decrypted device", g_no_log);
            return;

        }

        // close
        std::string close_cmd = "cryptsetup close " + mapper_name;
        auto close_res = EXEC_SUDO(close_cmd);

        if (!close_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to close mapper device");
            Logger::error("Failed to close mapper device", g_no_log);
            return;

        }

        // wipe passphrase from memory
        std::fill(passphrase.begin(), passphrase.end(), '\0');

        std::cout << GREEN << "\n[SUCCESS] " << RESET << "USB successfully locked and unmounted.\n";
        Logger::success("USB successfully locked and unmounted", g_no_log);
    }

    static void cryptionAltMenu(const std::string &drive_name) {
        std::cout << "\nDo you want to Encrypt or Decrypt your USB? (e/d):\n";

        char e_or_d;
        std::cin >> e_or_d;

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (!std::cin) {

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            ERR(ErrorCode::InvalidInput, "Input expected to be lower case single char 'n' or 'y'");
            Logger::error("invalid input " + std::string(1, e_or_d), g_no_log);
            return;
            
        }

        e_or_d = std::tolower(e_or_d);

        if (e_or_d != 'e' && e_or_d != 'd') {

            ERR(ErrorCode::InvalidInput, "Expected intput = 'd' or 'e'");
            Logger::error("invalid input ", g_no_log);
            return;

        }

        if (e_or_d == 'e') {

            encryptUSBDrive(drive_name);
            return;

        } else if (e_or_d == 'd') {

            decryptUSBDrive(drive_name);
            return;

        }
    }

public:
    static void mainUsbEnDecryption() {
        std::cout << "\nChoose your USB Drive you want to En/Decrypt\n";

        const std::string drive_name = ListDrivesUtil::listDrives(true);

        const auto val_drive_name = isValidDrive(drive_name);
    
        if (!val_drive_name.has_value()) {

            ERR(ErrorCode::InvalidDevice, "'" + drive_name + "' Couldnt get validated; Expected USB Drive");
            Logger::error("'" + drive_name + "' Couldnt get validated", g_no_log);
            return;

        }
    
        std::cout << YELLOW << "\n[Warning] " << RESET << "Are you sure you want to en- or decrypt: '" << drive_name << "' ? (y/N)\n";
    
        char confirmation = 'n';
        std::cin >> confirmation;

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (!std::cin) {

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            ERR(ErrorCode::InvalidInput, "Input expected to be lower case single char 'n' or 'y'");
            Logger::error("invalid input " + std::string(1, confirmation), g_no_log);
            return;
            
        }
        
        confirmation = std::tolower(confirmation);

        if (confirmation != 'n' && confirmation != 'y') {

            ERR(ErrorCode::InvalidInput, "Expected intput = 'n' or 'y'");
            Logger::error("invalid input " + std::string(1, confirmation), g_no_log);
            return;

        }

        bool is_confirm_key_true = confirmationKeyInput();

        if (!is_confirm_key_true) {

            Logger::info("ConfirmKeyInput is false, aborting operation", g_no_log);
            return;

        }

        if (confirmation == 'y') {

            std::cout << "\nProceeding...\n";
            cryptionAltMenu(drive_name);

        } else if (confirmation == 'n') {

            std::cout << YELLOW << "[INFO] " << RESET << "En- Decryption with '" << drive_name << "' was aborted by the user\n";
            Logger::info("En- Decryption with '" + drive_name + "' was aborted by the user", g_no_log);
            return;

        }
    }
};

// ========== Drive Data Overwriting ==========
// Tried my best to make this as safe and readable and maintainable as possible. v0.9.12.92

void overwriteDriveData() { 
    const std::string drive_to_operate_on = ListDrivesUtil::listDrives(true);

    std::cout << YELLOW << "[WARNING]" << RESET << " Are you sure you want to overwrite all data on " << BOLD << drive_to_operate_on << RESET << "? This action cannot be undone! (y/n)\n";
        
    auto confirm = validateCharInput({'y', 'n'});
    if (!confirm.has_value()) return;

    if (confirm != 'y') {

        std::cout << BOLD << "[Overwriting aborted]" << RESET << " The Overwriting process of " << drive_to_operate_on << " was interupted by user\n";
        Logger::info("Overwriting process aborted by user for drive: " + drive_to_operate_on, g_no_log);
        return;

    }

    std::cout << "\nTo be sure you want to overwrite the data on " << BOLD << drive_to_operate_on << RESET << " you need to enter the following safety key\n";

    std::string conf_key = confirmationKeyGenerator();
    Logger::info("Confirmation key generated for overwriting drive: " + drive_to_operate_on, g_no_log);

    std::cout << "\n" << conf_key << "\n";
    std::cout << "\nEnter the confirmation key:\n";

    std::string user_input;
    std::getline(std::cin, user_input);

    if (user_input != conf_key) {

        std::cout << BOLD << "[INFO]" << RESET << " The confirmationkey was incorrect, the overwriting process has been interupted\n";
        Logger::info("Incorrect confirmation key entered, overwriting process aborted for drive: " + drive_to_operate_on, g_no_log);
        return;

    }

    std::cout << YELLOW << "\n[Process]" << RESET << " Proceeding with overwriting all data on: " << drive_to_operate_on << "\n";
    std::cout << " \n";

    try {
        auto res_urandom = EXEC_SUDO("dd if=/dev/urandom of=" + drive_to_operate_on + " bs=8M status=progress && sync"); 
        auto res_zero = EXEC_SUDO("dd if=/dev/zero of=" + drive_to_operate_on + " bs=8M status=progress && sync"); 
            
        if (!res_urandom.success && !res_zero.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to overwrite the drive: " + drive_to_operate_on);
            Logger::info("Overwriting failed to complete for drive: " + drive_to_operate_on, g_no_log);
            return;

        } else if (!res_urandom.success || !res_zero.success) {

            std::cout << YELLOW << "[Warning]" << RESET << " One of the overwriting operations failed, but the drive may have been partially overwritten. Please check the output and try again if necessary.\n";
            Logger::warning("One of the overwriting operations failed for drive: " + drive_to_operate_on, g_no_log);
            return;
        } 

        std::cout << GREEN << "[Success]" << RESET << " Overwriting completed successfully for drive: " << drive_to_operate_on << "\n";
        Logger::success("Overwriting completed successfully for drive: " + drive_to_operate_on, g_no_log);
        return;

    } catch (const std::exception& e) {

        Logger::error("Failed during overwriting process for drive: " + drive_to_operate_on + " Reason: " + e.what(), g_no_log);
        ERR(ErrorCode::ProcessFailure, "Exception during overwriting process: " + std::string(e.what()));
        return;

    }
}

// ========== Drive Metadata Reader ==========
// getMetadata refactor

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

    static std::optional<DriveMetadata> getMetadata(const std::string& drive) {
        DriveMetadata metadata;
        // -P (Pairs) is the key here. It output KEY="VALUE"
        std::string cmd = "lsblk -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -P -p " + drive; 

        auto res = EXEC_QUIET(cmd);

        if (!res.success || res.output.empty()) { 

            ERR(ErrorCode::ProcessFailure, "The lsblk failed to deliver data");
            Logger::error("lsblk failed to deliver data -> getMetadata", g_no_log);
            return std::nullopt; 
            
        }

        auto extract = [&](const std::string& key) -> std::string {
            std::string search = key + "=\"";
            size_t start = res.output.find(search);

            if (start == std::string::npos) return "N/A";
            
            start += search.length();
            size_t end = res.output.find("\"", start);

            if (end == std::string::npos) return "N/A";
            
            std::string val = res.output.substr(start, end - start);
            return val.empty() ? "N/A" : val;
        };

        metadata.name       = extract("NAME");
        metadata.size       = extract("SIZE");
        metadata.model      = extract("MODEL");
        metadata.serial     = extract("SERIAL");
        metadata.type       = extract("TYPE");
        metadata.mountpoint = extract("MOUNTPOINT");
        metadata.vendor     = extract("VENDOR");
        metadata.fstype     = extract("FSTYPE");
        metadata.uuid       = extract("UUID");

        return metadata;
    }

    static void displayMetadata(const DriveMetadata& metadata) {
        std::cout << "\n-------- Drive Metadata --------\n";

        auto printAttr = [&](const std::string& attr, const std::string& value) {
            std::cout << "| " << attr << ": " << (value.empty() ? "N/A" : value) << "\n";
        };

        printAttr("Name", metadata.name);
        printAttr("Size", metadata.size);
        printAttr("Model", metadata.model);
        printAttr("Serial", metadata.serial);
        printAttr("Type", metadata.type);
        printAttr("Mountpoint", metadata.mountpoint.empty() ? "Not mounted" : metadata.mountpoint);
        printAttr("Vendor", metadata.vendor);
        printAttr("Filesystem", metadata.fstype);
        printAttr("UUID", metadata.uuid);

        if (metadata.type == "disk") {

            std::cout << "\n┌-─-─-─- SMART Data -─-─-─-─\n";
            
            std::string smartCmd = "smartctl -i " + metadata.name;
            auto res = EXEC_QUIET_SUDO(smartCmd); 

            if (!res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to retrieve SMART data for " + metadata.name);
                Logger::error("Failed to retrieve SMART data for " + metadata.name + " -> displayMetadata()", g_no_log);
                return;

            }

            std::string smartOutput = removeFirstLines(res.output, 4);

            if (!smartOutput.empty()) {

                std::cout << smartOutput;

            } else {

                ERR(ErrorCode::CorruptedData, "Failed to retrieve SMART data for " + metadata.name);
                Logger::error("Failed to retrieve SMART data for " + metadata.name + " -> displayMetadata()", g_no_log);

            }
        }   
        std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";         
    } 
    
public:
    static void mainReader() {
        const std::string driveName = ListDrivesUtil::listDrives(true);

        try {

            auto metadata = getMetadata(driveName);

            if (!metadata.has_value()) {

                ERR(ErrorCode::ProcessFailure, "Failed to read metadata for drive: " + driveName);
                Logger::error("Failed to read metadata for drive: " + driveName, g_no_log);
                return;
                
            }

            displayMetadata(*metadata);

            Logger::success("Successfully read metadata for drive: " + driveName, g_no_log);

        } catch (const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Failed to read drive metadata: " + std::string(e.what()));
            Logger::error("Failed to read drive metadata: " + std::string(e.what()), g_no_log);
            return;

        }
    } 
};

// ========== Mounting and Burning Utilities ==========
// IsoFileMetadataChecker and IsoBurner refactored; v0.9.13.93

class MountUtility {
private:
    /**
     * @brief Checks if the provided ISO file has valid metadata by verifying the ISO9660 signature at the expected offset. This helps ensure that the file is a valid ISO image before attempting to burn it to a storage device.
     * @param iso_path The file path to the ISO image to be checked.
     * @return true if the ISO file is valid and contains the correct metadata; false otherwise.
     */
    static bool IsoFileMetadataChecker(const std::string& iso_path) {
        namespace fs = std::filesystem;

        if (!fs::exists(iso_path) || !fs::is_regular_file(iso_path)) {

            ERR(ErrorCode::FileNotFound, "ISO file does not exist: " + iso_path);
            Logger::error("ISO file does not exist: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;

        }

        constexpr std::streamoff iso_magic_offset = 32769; // 16 * 2048 + 1

        if (fs::file_size(iso_path) < iso_magic_offset + 5) {

            ERR(ErrorCode::InvalidInput, "ISO file too small to contain valid metadata: " + iso_path);
            Logger::error("ISO file too small to contain valid metadata: " + iso_path, g_no_log);
            return false;

        }

        std::ifstream iso_file(iso_path, std::ios::binary);

        if (!iso_file) {

            ERR(ErrorCode::IOError, "Cannot open ISO file: " + iso_path);
            Logger::error("Cannot open ISO file: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;

        }

        iso_file.seekg(iso_magic_offset);

        char buffer[6] = {};
         
        if (!iso_file.read(buffer, 5)) {

            ERR(ErrorCode::IOError, "Failed to read ISO metadata from file: " + iso_path);
            Logger::error(" Failed to read ISO metadata: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;

        }

        // ISO9660 magic signature
        if (std::strncmp(buffer, "CD001", 5) != 0) {

            ERR(ErrorCode::InvalidInput, "Invalid ISO signature in file: " + iso_path);
            Logger::error("Invalid ISO signature in file: " + iso_path + " -> IsoFileMetadataChecker()", g_no_log);
            return false;

        }

        return true;
    }

    static void BurnISOToStorageDevice() {
        try {
            const std::string drive_name = ListDrivesUtil::listDrives(true);

            std::cout << "Enter the path to the ISO/IMG file you want to burn on " << drive_name << ":\n";

            std::string iso_path;
            std::getline(std::cin >> std::ws, iso_path);

            if (size_t pos = iso_path.find_first_of("-'&|<>;\""); pos != std::string::npos) {

                ERR(ErrorCode::InvalidInput, "Invalid characters in ISO path: " + iso_path);
                Logger::error("Invalid characters in ISO path\n", g_no_log);
                return;

            }

            if (!IsoFileMetadataChecker(iso_path)) {

                ERR(ErrorCode::InvalidInput, "Invalid ISO file: " + iso_path);
                Logger::error("Invalid ISO file: " + iso_path + " -> BurnISOToStorageDevice()", g_no_log);
                return;

            }

            std::cout << "Are you sure you want to burn " << iso_path << " to " << drive_name << "? (y/n)\n";

            auto confirmation = validateCharInput({'y', 'n'});
            if (!confirmation.has_value()) return;

            if (confirmation != 'y') {

                std::cout << YELLOW << "[INFO] Operation cancelled\n" << RESET;
                Logger::info("Burn operation cancelled by user -> BurnISOToStorageDevice()", g_no_log);
                return;

            }

            std::string confirmation_key = confirmationKeyGenerator();
            std::cout << "\nEnter the confirmation key to proceed:\n";
            std::cout << confirmation_key << "\n";
                        
            std::string user_key_input;
            std::getline(std::cin, user_key_input);

            if (user_key_input != confirmation_key) {

                ERR(ErrorCode::InvalidInput, "Incorrect confirmation key.");
                Logger::error("Incorrect confirmation key -> BurnISOToStorageDevice()", g_no_log);
                return;

            }

            std::cout << "\n" << YELLOW << "[PROCESS] Burning ISO to device...\n" << RESET;

            auto unmount_res = EXEC_SUDO("umount " + drive_name + "* 2>/dev/null || true");
            
            if (!unmount_res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to unmount drive: " + drive_name);
                Logger::error("dd burn failed for drive: " + drive_name + " -> BurnISOToStorageDevice()", g_no_log);
                return;  

            }

            auto res = EXEC_SUDO("dd if=" + iso_path + " of=" + drive_name + " bs=4M status=progress && sync"); 

            if (!res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to burn ISO: " + res.output);
                Logger::error("dd burn failed for drive: " + drive_name + " -> BurnISOToStorageDevice()", g_no_log);
                return;

            }

            std::cout << GREEN << "[SUCCESS] Successfully burned " << iso_path << " to " << drive_name << "\n" << RESET;

            Logger::success("Successfully burned ISO to drive: " + drive_name + " -> BurnISOToStorageDevice()", g_no_log);

        } catch (const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Burn operation failed: " + std::string(e.what()));
            Logger::error("BurnISOToStorageDevice() exception: " + std::string(e.what()), g_no_log);
            return;

        }
    }

    /**
     * @brief wrapps the orgirnal unmount() and mount() funcs to gether in one
     * @param mount_or_unmount type in mount, you will get the mount function, type in unmount you will get the unmount fukntion
     */
    static void choose_mount_unmount(const std::string &mount_or_unmount) {
        if (mount_or_unmount == "mount") {

            std::cout << "Enter the drive you want to " << mount_or_unmount << "\n";
            const std::string drive_name = ListDrivesUtil::listDrives(true);

            std::cout << "\nEnter the name for the drive under the name its mounted under '/mnt/':\n";

            std::string mount_name;
            std::getline(std::cin, mount_name);

            std::string mount_cmd = "mount " + drive_name + " /mnt/" + mount_name;
            auto mount_res = EXEC_SUDO(mount_cmd);
             
            if (!mount_res.success) {

                ERR(ErrorCode::ProcessFailure, "Couldnt mount '" + drive_name + "' at '/mnt/" + mount_name);
                Logger::error("Couldnt mount '" + drive_name + "' at '/mnt/" + mount_name, g_no_log);
                return;

            }

        } else if (mount_or_unmount == "unmount") {

            std::cout << "\nEnter the name, under wich the target drive is mounted (it maby under '/mnt/' or '/media/<user>/'):\n";

            std::string unmount_name;
            std::getline(std::cin, unmount_name);

            if (!unmount_name.find("/mnt/") && !unmount_name.find("/media/")) {

                ERR(ErrorCode::InvalidDevice, "The path you entered doesnt contain /mnt/ or /media/, what results in an failing unmount operation");
                Logger::error("The path you entered doesnt contain /mnt/ or /media/, what results in an failing unmount operation", g_no_log);
                return;

            }

            std::string unmount_cmd = "umount " + unmount_name;
            auto unmount_res = EXEC_SUDO(unmount_cmd);
             
            if (!unmount_res.success) {

                ERR(ErrorCode::ProcessFailure, "Couldnt unmount " + unmount_name);
                Logger::error("Couldnt unmount " + unmount_name, g_no_log);
                return;

            }
        }

        return;
    }

    static void Restore_USB_Drive() {
        const std::string restore_device_name = ListDrivesUtil::listDrives(true);
        try {

            std::cout << "Are you sure you want to overwrite/clean the ISO/Disk_Image from: " << restore_device_name << " ? [y/n]\n";
            
            auto restore_confirm = validateCharInput({'y', 'n'});
            if (!restore_confirm.has_value()) return;

            if (restore_confirm != 'y') {
                std::cout << CYAN << "[INFO] " << RESET << "Operation cancelled\n";
                Logger::info("restore usb operation cancelled", g_no_log);
                return;

            }

            EXEC_QUIET_SUDO("umount " + restore_device_name + "* 2>/dev/null || true");
            
            auto wipefs_res = EXEC_SUDO("wipefs -a " + restore_device_name);

            if (!wipefs_res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to wipe device: " + restore_device_name);
                Logger::error("Failed to wipe the filesystem of " + restore_device_name, g_no_log);
                return;

            }

            // Zero out start
            auto dd_res = EXEC_SUDO("dd if=/dev/zero of=" + restore_device_name + " bs=1M count=10 status=progress && sync");

            if (!dd_res.success) {

                Logger::error("Failed to overwrite the iso image on the usb -> Restore_USB_Drive()", g_no_log);
                ERR(ErrorCode::ProcessFailure, "Failed to overwrite device: " + restore_device_name);
                return;

            }

            // Create partition table
            auto parted_res = EXEC_SUDO("parted -s " + restore_device_name + " mklabel msdos mkpart primary 1MiB 100%");

            if (!parted_res.success) {

                Logger::error("Failed while restoring USB: " + restore_device_name, g_no_log);
                ERR(ErrorCode::ProcessFailure, "Failed to create partition table on USB device: " + restore_device_name);
                return;

            }

            // Probe partitions
            auto partprobe_res = EXEC_SUDO("partprobe " + restore_device_name);

            if (!partprobe_res.success) { 

                ERR(ErrorCode::ProcessFailure, "Couldnt partition the drive: " + restore_device_name);
                Logger::error("Couldnt partition the drive: " + restore_device_name, g_no_log);
                return;

            }

            std::string partition_path = restore_device_name;

            if (!partition_path.empty() && std::isdigit(partition_path.back())) {

                partition_path += "p1"; 
            
            } else { 

                partition_path += "1";

            }

            auto mkfs_res = EXEC_SUDO("mkfs.vfat -F32 " + partition_path);

            if (!mkfs_res.success) {

                Logger::error("Failed while formatting USB: " + restore_device_name, g_no_log);
                ERR(ErrorCode::ProcessFailure, "Failed to format USB device with FS: " + restore_device_name);
                return;

            }

            std::cout << GREEN << "[Success] Your USB should now function as a normal FAT32 drive (partition: " << partition_path << ")\n" << RESET;
            Logger::success("Restored USB device " + restore_device_name + " -> formatted " + partition_path, g_no_log);
            return;

        } catch (const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Failed to initialize usb restore function: " + std::string(e.what()));
            Logger::error("failed to initialize restore usb function", g_no_log);
            return;

        }
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
        std::cout << "│0. Return to main menu              │\n";
        std::cout << "└────────────────────────────────────┘\n";

        auto menu_input = validateIntInput(0, 4);
        if (!menu_input.has_value()) return;
        
        switch (*menu_input) {
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

                ERR(ErrorCode::OutOfRange, "Invalid menu option selected");
                return;

            }
        }
    }
};

// ========== Forensic Analysis Utilities ==========

class ForensicAnalysis {
private:
    static void CreateDiskImage() {
        try {
            const std::string driveName = ListDrivesUtil::listDrives(true);

            std::cout << "Enter the path where the disk image should be saved (e.g., /path/to/image.img):\n";
            std::string imagePath;
            std::cin >> imagePath;
            std::cout << "Are you sure you want to create a disk image of " << driveName << " at " << imagePath << "? (y/n)\n";
            char confirmationcreate;
            std::cin >> confirmationcreate;

            if (confirmationcreate != 'y' && confirmationcreate != 'Y') {
                std::cout << "[Info] Operation cancelled\n";
                Logger::info("Operation cancelled -> CreateDiskImage()", g_no_log);
                return;
            }

            auto res = EXEC_SUDO("dd if=" + driveName + " of=" + imagePath + " bs=4M status=progress && sync");
            if (!res.success) {
                ERR(ErrorCode::ProcessFailure, "Failed to create disk image");
                Logger::error("Failed to create disk image for drive: " + driveName + " -> CreateDiskImage()", g_no_log);
                return;
            }

            std::cout << GREEN << "[Success] Disk image created at " << imagePath << "\n" << RESET;
            Logger::success("Disk image created successfully for drive: " + driveName + " -> CreateDiskImage()", g_no_log);
       
        } catch (const std::exception& e) {
            ERR(ErrorCode::ProcessFailure, "Failed to create Disk image: " + std::string(e.what()));
            Logger::error("Failed to create disk image: " + std::string(e.what()), g_no_log);
            return;
        }
    }

    // recoverymain + side functions
    static void recovery() {
        std::cout << "\n-------- Recovery ---------−−−\n";
        std::cout << "1. files recovery\n";
        std::cout << "2. partition recovery\n";
        std::cout << "3. system recovery\n";
        std::cout << "0. Return to main menu\n";
        std::cout << "--------------------------------\n";
        std::cout << "Enter your choice:\n"; 
        
        auto scan_drive_recover = validateIntInput(0, 3);
        if (!scan_drive_recover.has_value()) return;

        switch (*scan_drive_recover) {
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

            case 0: {
                return;
            }

            default: {
                ERR(ErrorCode::OutOfRange, "Invalid recovery option selected");  
                break;     
            }  
        }   
    }

    //·−−− recovery side functions
    static void filerecovery() {
        //std::string device = getAndValidateDriveName("Enter the NAME of a drive or image to scan for recoverable files (e.g., /dev/sda:");
        const std::string device = ListDrivesUtil::listDrives(true);

        static const std::vector<std::string> signature_names = {"all","png","jpg","elf","zip","pdf","mp3","mp4","wav","avi","tar.gz","conf","txt","sh","xml","html","csv"};
        std::cout << "Type signature to search (e.g. png) or 'all':\n";

        std::string sig_in;
        std::cin >> sig_in;

        size_t sig_idx = SIZE_MAX;

        for (size_t i = 0; i < signature_names.size(); ++i) if (signature_names[i] == sig_in) { sig_idx = i; break; }
        if (sig_idx == SIZE_MAX) { ERR(ErrorCode::InvalidInput, "Unsupported signature: " + sig_in); return; }

        std::cout << "Scan depth: 1=quick 2=full\n";

        auto depth = validateIntInput({1, 2});
        if (!depth.has_value()) return;

        if (depth == 1) file_recovery_quick(device, (int)sig_idx);
        else if (depth == 2) file_recovery_full(device, (int)sig_idx);
    }

    static void file_recovery_quick(const std::string& drive, int signature_type) {
        std::cout << "Scanning drive for recoverable files (quick) - signature index: " << signature_type << "...\n";

        // Mapping of the numeric menu choices to signature keys
        static const std::vector<std::string> signature_names = {
            "all", "png", "jpg", "elf", "zip", "pdf", "mp3", "mp4", "wav", "avi",
            "tar.gz", "conf", "txt", "sh", "xml", "html", "csv"
        };

        if (signature_type < 0 || static_cast<size_t>(signature_type) >= signature_names.size()) {
            ERR(ErrorCode::InvalidInput, "Invalid signature type index: " + std::to_string(signature_type));
            return;
        }

        const std::string key = signature_names[signature_type];

        // Helper: scan a signature over the drive, limited to max_blocks (SIZE_MAX = full)
        auto scan_signature = [&](const file_signature& sig, size_t max_blocks) {
            if (sig.header.empty()) return;

            const size_t block_size = 4096;
            std::ifstream disk(drive, std::ios::binary);

            if (!disk.is_open()) {
                ERR(ErrorCode::DeviceNotFound, "Cannot open drive/image: " + drive);
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
                ERR(ErrorCode::InvalidInput, "Signature not found: " + key);
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
            ERR(ErrorCode::InvalidInput, "Invalid signature type index: " + std::to_string(signature_type));
            return;
        }

        const std::string key = signature_names[signature_type];

        auto scan_signature_full = [&](const file_signature& sig) {
            if (sig.header.empty()) return;
            const size_t block_size = 4096;
            std::ifstream disk(drive, std::ios::binary);

            if (!disk.is_open()) {
                ERR(ErrorCode::DeviceNotFound, "Cannot open drive/image: " + drive);
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
                ERR(ErrorCode::InvalidInput, "Signature not found: " + key);
                return;
            }
            scan_signature_full(it->second);
        }
    }

    static void partitionrecovery() {
        std::string device = ListDrivesUtil::listDrives(true);

        std::cout << "\n--- Partition table (parted) ---\n";
        auto parted_res = EXEC_SUDO("parted -s " + device + " print");
        
        std::cout << "\n--- fdisk -l ---\n";
        auto fdisk_res = EXEC_SUDO("fdisk -l " + device + " 2>/dev/null || true");

        // Offer to dump partition table using sfdisk (non-destructive)
        std::cout << "Would you like to save a partition-table dump (recommended) to a file for possible restoration? (y/N): ";
        auto save_dump = validateCharInput({'y', 'n'});
        if (!save_dump.has_value()) return;

        std::string dumpPath;

        if (save_dump == 'y') {
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
                Logger::info("Partition-table dump saved: " + dumpPath + " for device: " + device + " -> partitionrecovery()", g_no_log);
           
            } else {

                std::cout << "[Warning] Could not write partition-table dump.\n";
                Logger::warning("Failed to write partition-table dump for device: " + device + " -> partitionrecovery()", g_no_log);
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
        std::string device = ListDrivesUtil::listDrives(true);

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
            ERR(ErrorCode::ProcessFailure, "Could not create system recovery helper script at path: " + scriptPath);
            Logger::error("Could not create system recovery script: " + scriptPath + " -> systemrecovery()", g_no_log);
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
            Logger::warning("User allowed DriveMgr to run system recovery helper script: " + scriptPath + " -> systemrecovery()", g_no_log);
            
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

        std::cout << "\n┌──────── Forensic Analysis menu ─────────┐\n";
        std::cout << "│ 1. Info of the Analysis tool            │\n";
        std::cout << "│ 2. Create a disk image of a drive       │\n";
        std::cout << "│ 3. recover of system, files,..          │\n";                                                                                                                                                                                                                                                                                                                                                                                                        
        std::cout << "│ 0. Exit                                 │\n";                                                                                                                                                                                                                                                                                                                                                                                               
        std::cout << "└─────────────────────────────────────────┘\n";
        
        auto forensic_menu_input = validateIntInput(0, 3);
        if (!forensic_menu_input.has_value()) return;

        switch (static_cast<ForensicMenuOptions>(*forensic_menu_input)) {
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
                ERR(ErrorCode::OutOfRange, "Invalid menu selection in forensic analysis menu");
                return;
            }
        }
    }
};

// ========== Clone Drive Utility ==========

class Clone {
    private:
        static void CloneDrive(const std::string &source, const std::string &target) {
            std::cout << "\n[CloneDrive] Do you want to clone data from " << source << " to " << target << "? This will overwrite all data on the target drive(n) (y/n): ";
            
            auto confirmation = validateCharInput({'y', 'n'});
            if (!confirmation.has_value()) return;

            if (confirmation != 'y') {

                std::cout << "[Info] Operation cancelled\n";
                Logger::info("Operation cancelled -> Clone::CloneDrive()", g_no_log);
                return;

            } else if (confirmation == 'y') {
                try {

                    auto res = EXEC_SUDO("dd if=" + source + " of=" + target + " bs=5M status=progress && sync");

                    if (!res.success) {
                        Logger::error("Failed to clone drive from " + source + " to " + target + " -> Clone::CloneDrive()", g_no_log);
                        ERR(ErrorCode::ProcessFailure, "Failed to clone data from " + source + " to " + target);
                        return;
                    }

                    std::cout << GREEN << "[Success] Drive cloned from " << source << " to " << target << "\n" << RESET;
                    Logger::success("Drive cloned successfully from " + source + " to " + target + " -> Clone::CloneDrive()", g_no_log);
                
                } catch (const std::exception& e) {

                    ERR(ErrorCode::ProcessFailure, "Failed to clone drive: " + std::string(e.what()));
                    Logger::error(std::string("Failed to clone drive from ") + source + " to " + target + " -> Clone::CloneDrive(): " + e.what(), g_no_log);
                    return;

                }
            }

        }

        static std::optional<std::string> validateTargetDriveName(const std::string& target_drive) {
            static const std::array<std::string, 3> valid_paths_contains {
                "/mnt/", "/dev/", "/media/"
            };

            if (target_drive.empty()) {

                ERR(ErrorCode::DataUnavailable, "Target drive cannot be empty string");
                Logger::error("Target drive cannot be empty string", g_no_log);
                return std::nullopt;

            }

            for (const auto& path : valid_paths_contains) {

                if (target_drive.find(path) != std::string::npos) {

                    return target_drive;

                }

            }

            ERR(ErrorCode::InvalidInput, "Target drive string doesn't contain a valid drive path prefix");
            return std::nullopt;
        }

        
    public:
        static void mainClone() {
            try {
                std::cout << "\nChoose a Source drive to clone the data from it:\n";
                const std::string source_drive = ListDrivesUtil::listDrives(true);

                std::cout << "\nEnter a Target drive/device to clone the data on to it (dont choose the same drive):\n";
                std::cout << YELLOW << "[WARNING]" << RESET << " Make sure to choose the mount path of the target" << BOLD << " (e.g., /media/target_drive)\n" << RESET;
                
                std::string target_drive;
                std::getline(std::cin, target_drive);

                const auto validated = validateTargetDriveName(target_drive);
                if (!validated) { return; }

                const std::string val_target = *validated;

                if (source_drive == val_target) {

                    Logger::error("Source and target drives are the same -> Clone::mainClone()", g_no_log);
                    ldm_runtime_error("[ERROR] Source and target drives cannot be the same!");
                    return;

                } else {

                    CloneDrive(source_drive, val_target);
                    return;
                }

            } catch (std::exception& e) {

                ERR(ErrorCode::ProcessFailure, "An error occurred during the clone initializing process: " + std::string(e.what()));
                Logger::error(std::string(e.what()), g_no_log);
                return; 

            }
        }
};

// ========== Log Viewer Utility ==========

void logViewer() {
    std::string path = filePathHandler("/.local/share/DriveMgr/data/log.dat");

    std::ifstream file(path);

    if (!file) {

        Logger::error("Unable to read log file at " + path, g_no_log);
        ERR(ErrorCode::FileNotFound, "Unable to read log file at path: " + path);
        std::cout << "Please read the log file manually at: " << path << "\n";
        return;

    }

    std::cout << "\nLog file content:\n";

    const std::unordered_map<std::string, std::string> log_tags {
        {"[ERROR]", RED},
        {"[EXEC]",  CYAN},
        {"[WARNING]", YELLOW},
        {"[DRY-RUN]", MAGENTA},
        {"[SUCCESS]", GREEN}
    };

    std::string line;

    while (std::getline(file, line)) {
        bool matching = false;

        for (const auto& [log_tag, color] : log_tags) {

            if (line.find(log_tag) != std::string::npos) {

                std::cout << color << line << RESET << "\n";

                matching = true;

                break;
            }
        }

        if (matching != true) {
            std::cout << line << "\n";
        }
    }

    std::cout << "\nDo you want to empty the log file content? (y/n):\n";
    
    auto clear_loggs = validateCharInput({'y', 'n'});
    if (!clear_loggs.has_value()) return;

    if (clear_loggs == 'y') {

        Logger::clearLoggs(path);

    }

    return;
}

// ========== Configuration Editor Utility ==========
// v0.9.19.23; added fallbacks for config values if the user doesnt specify them in the config file

class ConfigValueHandeling {
    public:
        struct CONFIG_VALUES {
            std::string UI_MODE = "CLI";
            std::string COMPILE_MODE = "StatBin";
            std::string THEME_COLOR_MODE = "RESET";
            std::string SELECTION_COLOR_MODE = "RESET";
            bool DRY_RUN_MODE = false;
            bool ROOT_MODE = false;
        };

        static CONFIG_VALUES configHandler() {
            CONFIG_VALUES cfg{}; 
            std::string conf_file = filePathHandler("/.local/share/DriveMgr/data/config.conf");

            if (conf_file.empty()) {

                ERR(ErrorCode::DataUnavailable, "Using default config values!");
                Logger::error("config file is using defautl values, due to emtpy config", g_no_log);
                return cfg;

            }

            std::ifstream config_file(conf_file);

            if (!config_file.is_open()) {

                Logger::error("[Config_handler] Cannot open config file", g_no_log);
                ERR(ErrorCode::FileNotFound, "Cannot open config file at path: " + conf_file + ". Check if the config exists and is readable. Returning default config values.");
                return cfg;

            }

            std::string line;

            while (std::getline(config_file, line)) {
                if (line.empty() || line[0] == '#') { continue; }

                size_t pos = line.find('=');
                if (pos == std::string::npos) { continue; }

                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);

                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "UI_MODE") cfg.UI_MODE = value;
                else if (key == "COMPILE_MODE") cfg.COMPILE_MODE = value;
                else if (key == "COLOR_THEME") cfg.THEME_COLOR_MODE = value;
                else if (key == "SELECTION_COLOR") cfg.SELECTION_COLOR_MODE = value;
                else if (key == "DRY_RUN_MODE") {
                    std::string v = value;
                    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                    cfg.DRY_RUN_MODE = (v == "true");
                }
                else if (key == "ROOT_MODE") {
                    std::string v = value;
                    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                    cfg.ROOT_MODE = (v == "true");
                };
            }
            return cfg;
        }

        /**
         * Helper function to check if a file exists at the given path.
         * @param path The file path to check.
         * @return true if the file exists, false otherwise.
         */
        static bool fileExists(const std::string& path) { struct stat buffer; return (stat(path.c_str(), &buffer) == 0); }

        static void configEditor() {
            CONFIG_VALUES cfg = configHandler();
            
            std::cout << "┌─────" << BOLD << " config values " << RESET << "─────┐\n";
            std::cout << "│ UI mode: "          << cfg.UI_MODE               << "\n";
            std::cout << "│ Compile mode: "     << cfg.COMPILE_MODE          << "\n";
            std::cout << "│ Dry run mode: "     << cfg.DRY_RUN_MODE          << "\n";
            std::cout << "│ Root mode: "        << cfg.ROOT_MODE             << "\n";
            std::cout << "│ Theme Color: "      << cfg.THEME_COLOR_MODE      << "\n";
            std::cout << "│ Selection Color: "  << cfg.SELECTION_COLOR_MODE  << "\n";
            std::cout << "└─────────────────────────┘\n";   
            std::cout << "\nDo you want to edit the config file? (y/n)\n";
            
            auto config_edit_confirm = validateCharInput({'y', 'n'}); 
            if (!config_edit_confirm.has_value()) return; 

            if (config_edit_confirm == 'y') {

                std::string lumePath = filePathHandler("/.local/share/DriveMgr/bin/Lume/Lume");

                if (!fileExists(lumePath)) {

                    ERR(ErrorCode::FileNotFound, "Lume editor not found at: " + lumePath);
                    return;

                }

                std::string configPath = filePathHandler("/.local/share/DriveMgr/data/config.conf");
                std::string config_editor_cmd_run = "\"" + lumePath + "\" \"" + configPath + "\"";

                std::cout << LEAVETERMINALSCREEN << std::flush;

                term.restoreTerminal();

                system(config_editor_cmd_run.c_str());

                std::cout << NEWTERMINALSCREEN << std::flush;

                term.enableRawMode();
                return;
            }        
        }

        static void colorThemeHandler() {
            CONFIG_VALUES cfg = configHandler();

            g_THEME_COLOR = RESET;
            g_SELECTION_COLOR = RESET;

            auto theme_color = available_colores.find(cfg.THEME_COLOR_MODE);

            if (theme_color != available_colores.end()) {

                g_THEME_COLOR = theme_color->second;

            }

            auto selection_color = available_colores.find(cfg.SELECTION_COLOR_MODE);

            if (selection_color != available_colores.end()) {

                g_SELECTION_COLOR = selection_color->second;

            }
        }
};

// ========== Drive Fingerprinting Utility ==========
// v0.9.19.24; applyed new ERR error handling

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
        std::string cmd = "lsblk -o NAME,SIZE,MODEL,SERIAL,UUID -P -p " + drive; 

        auto res = EXEC_QUIET(cmd);

        if (!res.success || res.output.empty()) { 

            ERR(ErrorCode::ProcessFailure, "The lsblk failed to deliver data");
            Logger::error("lsblk failed to deliver data -> getMetadata", g_no_log);
            return metadata; 

        }

        auto extract = [&](const std::string& key) -> std::string {
            std::string search = key + "=\"";
            size_t start = res.output.find(search);

            if (start == std::string::npos) return "N/A";
            
            start += search.length();
            size_t end = res.output.find("\"", start);

            if (end == std::string::npos) return "N/A";
            
            std::string val = res.output.substr(start, end - start);
            return val.empty() ? "N/A" : val;
        };

        metadata.name       = extract("NAME");
        metadata.size       = extract("SIZE");
        metadata.model      = extract("MODEL");
        metadata.serial     = extract("SERIAL");
        metadata.uuid       = extract("UUID");

        return metadata;
    }

    /**
     * @brief fingerprinting() takes the string combined_metadata and creates a sha256 hash of the combined_metadata
     * @param combined_metadata contains the metadata of the drive to create the sha256 has
     */
    static std::string fingerprinting(const std::string& combined_metadata) {
        unsigned char hash[SHA256_DIGEST_LENGTH];

        SHA256(reinterpret_cast<const unsigned char*>(combined_metadata.c_str()), combined_metadata.size(), hash);

        std::string fingerprint;

        debug_msg("going to generate fingerpint based on metadata", g_debug);

        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {

            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", hash[i]);
            fingerprint += hex;

        }
        
        if (fingerprint.empty()) {

            Logger::error("Failed to generate fingerprint -> DriveFingerprinting::fingerprinting()", g_no_log);
            ERR(ErrorCode::ProcessFailure, "Failed to generate fingerprint");
            return "";

        }

        debug_msg(fingerprint, g_debug);
        return fingerprint;
    }


public:
    static void fingerprinting_main() {
        const std::string drive_name_fingerprinting = ListDrivesUtil::listDrives(true);

        DriveMetadata metadata = getMetadata(drive_name_fingerprinting);

        Logger::info("Retrieved metadata for drive: " + drive_name_fingerprinting + " -> DriveFingerprinting::fingerprinting_main()", g_no_log);

        std::string combined_metadata =
            metadata.name + "|" +
            metadata.size + "|" +
            metadata.model + "|" +
            metadata.serial + "|" +
            metadata.uuid;

        std::string fingerprint = fingerprinting(combined_metadata);

        Logger::info("[INFO] Generated fingerprint for drive: " + drive_name_fingerprinting + " -> DriveFingerprinting::fingerprinting_main()", g_no_log);

        std::cout << BOLD << "Fingerprint:\n" << RESET;
        std::cout << "\r\033[K\n";
        std::cout << fingerprint << "\n";
    }
};

// ========== Main Menu and Utilities ==========

void Info() {
    int setw_for_version;
    if (VERSION.find_last_of("_dev")) { setw_for_version = 98; } else { setw_for_version = 102; }
    std::cout << "\n┌────────────────────────────────────────────────────────" << BOLD << " Info " << RESET << "────────────────────────────────────────────────────────┐\n";
    std::cout << "│ Welcome to Linux Drive Manager (DMgr / LDM) — a program for Linux to view and operate your storage devices." <<                   std::setw(14) << "│\n"; 
    std::cout << "│ Warning! You should know the basics about drives so you don't lose any data." <<                                                  std::setw(45) << "│\n";
    std::cout << "│ If you find problems or have ideas, visit the GitHub page and open an issue." <<                                                  std::setw(45) << "│\n";
    std::cout << "│ " << BOLD << "Other info:" << RESET <<                                                                                           std::setw(110) << "│\n";
    std::cout << "│ Version: " << BOLD << VERSION << RESET <<                                                                           std::setw(setw_for_version) << "│\n";
    std::cout << "│ Github: " << BOLD << "https://github.com/Dogwalker-kryt/Linux-Drive-Manager" << RESET <<                                          std::setw(60) << "│\n";
    std::cout << "│ Author: " << BOLD << "Dogwalker-kryt" << RESET <<                                                                                 std::setw(99) << "│\n";
    std::cout << "└──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n";
}

// v0.9.19.28 last change:
// added input and terminator for the alt terminal screen
static void printUsage(const char* progname) {
    std::cout << "Usage: " << progname << " [options]\n";
    std::cout << BOLD << "Options:\n" << RESET 
              << "  --version, -v       Print program version and exit\n"
              << "  --help, -h          Show this help and exit\n"
              << "  -n, --dry-run       Do not perform destructive operations\n"
              << "  -C, --no-color      Disable colors (may affect the main menu)\n"
              << "  --no-log            Disables all logging in the current session\n"
              << "  --debug             Enables debug messages in current session\n"
              << "  --operation-name    Goes directly to a specific operation without menu\n"
              << "                      Available operations:\n"
              << "                        --list-drives\n"
              << "                        --format-drive\n"
              << "                        --encrypt-decrypt\n"
              << "                        --resize-drive\n"
              << "                        --check-drive-health\n"
              << "                        --analyze-disk-space\n"
              << "                        --overwrite-drive-data\n"
              << "                        --view-metadata\n"
              << "                        --info\n"
              << "                        --forensics\n"
              << "                        --clone-drive\n";

    char input[2];
    std::cout << "press any key to close\n";
    fgets(input, sizeof(input), stdin);
    std::cout << LEAVETERMINALSCREEN;
}

/**
 * @brief This is the End question that is promted when a function failed/finished
 * @param running There is a variable in main that is named 'running', use it for only this
 */
void menuQues(bool& running) {   
    std::cout << BOLD <<"\nPress '1' for returning to the main menu, '2' to exit:\n" << RESET;

    auto menuques = validateIntInput({1, 2});

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
        Logger::error("Attempted to run without root privileges -> checkRoot()", g_no_log);
        return false;
    }
    return true;
}

bool checkRootMetadata() {
    if (!isRoot()) {
        std::cerr << YELLOW << "[WARNING] Running without root may limit functionality. For full access, please run with 'sudo'.\n" << RESET;
        Logger::warning("Running without root privileges -> checkRootMetadata()", g_no_log);
        return false;
    }
    return true;
}

enum MenuOptionsMain {
    EXITPROGRAM = 0,        LISTDRIVES = 1,         FORMATDRIVE = 2,        ENCRYPTDECRYPTDRIVE = 3,    RESIZEDRIVE = 4, 
    CHECKDRIVEHEALTH = 5,   ANALYZEDISKSPACE = 6,   OVERWRITEDRIVEDATA = 7, VIEWMETADATA = 8,           VIEWINFO = 9,
    MOUNTUNMOUNT = 10,      FORENSIC = 11,          LOGVIEW = 12,           CLONEDRIVE = 13,            CONFIG = 14,          
    FINGERPRINT = 15,       UPDATER = 16
};

class MainMenuIO {
    private:
        static void turnOffColor() {
            g_SELECTION_COLOR = RESET;
            g_THEME_COLOR = RESET;
            RED = RESET;
            CYAN = RESET;
            YELLOW = RESET;
            GREEN = RESET;
            MAGENTA = RESET;
            BLUE = RESET;
        }

    public:
        /**
         * @brief Its the menu Tui selection with colors
         * @param menuItems its defined in the main functions, it contains all avilable menu items
         */
        static int colorTuiMenu(const std::vector<std::pair<int, std::string>> &menuItems) {
            term.enableRawMode();

            int selected = 0;
            while (true) {
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

                    // Apply inverse only to inner content
                    if ((int)i == selected) std::cout << INVERSE;
                    std::cout << innerStr;
                    if ((int)i == selected) std::cout << RESET;

                    // Print right border and newline
                    std::cout << g_THEME_COLOR << " │\n" << RESET;

                }
                std::cout  << g_THEME_COLOR << "└─────────────────────────────────────────────────┘\n" << RESET;

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

            term.restoreTerminal();
            return selected;
            
        }

        /**
         * @brief Same shit as colorTuiMenu, but with no colors and ">" cursor
         * @param its defined in the main functions, it contains all avilable menu items
         */
        static int noColorTuiMenu(const std::vector<std::pair<int, std::string>> &menuItems) {
            turnOffColor();
            term.enableRawMode();

            int selected = 0;
            while (true) {
                auto res = EXEC("clear"); 
                std::cout << "Use Up/Down arrows and Enter to select an option.\n\n";
                std::cout << "┌─────────────────────────────────────────────────────┐\n";
                std::cout << "│" << BOLD << "                DRIVE MANAGEMENT UTILITY             " << RESET << "│\n";
                std::cout << "├─────────────────────────────────────────────────────┤\n";

                for (size_t i = 0; i < menuItems.size(); ++i) {

                    std::cout << "│ ";

                    // Cursor arrow
                    if ((int)i == selected)
                        std::cout << "> ";
                    else
                        std::cout << "  ";

                    // Build inner content with fixed width
                    std::ostringstream inner;
                    inner << std::setw(2) << menuItems[i].first << ". "
                        << std::left << std::setw(44) << menuItems[i].second;

                    std::cout << inner.str();

                    std::cout << "  │\n";
                }

                std::cout << "└─────────────────────────────────────────────────────┘\n";

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
                    selected = (int)menuItems.size() - 1; // Exit item index
                    break;
                }
            }

            term.restoreTerminal();
            return selected; 
        }

};

// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    std::cout << NEWTERMINALSCREEN;

    ConfigValueHandeling::colorThemeHandler();

    const std::map<std::string, std::function<void()>> cli_commands = {
        {"--list-drives", []()          { if (!checkRoot()) return; ListDrivesUtil::listDrives(false); }},
        {"--format-drive", []()         { if (!checkRoot()) return; formatDrive(); }},
        {"--encrypt-decrypt", []()      { if (!checkRoot()) return; USBEnDeCryptionUtils::mainUsbEnDecryption(); }}, 
        {"--resize-drive", []()         { if (!checkRoot()) return; resizeDrive(); }},
        {"--check-drive-health", []()   { if (!checkRoot()) return; checkDriveHealth(); }},
        {"--analyze-disk-space", []()   { analyzeDiskSpace(); }},
        {"--overwrite-drive-data", []() { if (!checkRoot()) return; overwriteDriveData(); }},
        {"--view-metadata", []()        { if (!checkRootMetadata()) return; MetadataReader::mainReader(); }},
        {"--info", []()                 { Info(); }},
        {"--forensics", []()            { if (!checkRoot()) return; ForensicAnalysis::mainForensic(); }},
        {"--clone-drive", []()          { if (!checkRoot()) return; Clone::mainClone(); }},
        {"--fingerprint", []()          { if (!checkRootMetadata()) return; DriveFingerprinting::fingerprinting_main(); }}
    };

    for (int i = 1; i < argc; i++) {
        std::string a(argv[i]); 

        if (a == "--no-color" || a == "--no_color" || a == "-c")    { g_no_color = true; continue; }

        if (a == "--no-log" || a == "-nl")                          { g_no_log = true; continue; }

        if (a == "--help" || a == "-h")                             { printUsage(argv[0]); return 0; }

        if (a == "--version" || a == "-v")                          { std::cout << "DriveMgr CLI version: " << VERSION << "\n"; return 0; }
        
        if (a == "--debug" || a == "-d")                            { g_debug = true; continue; }

        if (a == "--logs" || a == "-l")                             { logViewer(); return 0; }

        if (a == "--dry-run" || a == "-n")                          { g_dry_run = true; continue; }

        else {
            auto cmd = cli_commands.find(argv[i]);
            
            if (cmd != cli_commands.end()) {
                cmd->second();
                return 0;
            }
        }
    }

    ConfigValueHandeling::CONFIG_VALUES cfg = ConfigValueHandeling::configHandler();
    bool dry_run_mode = cfg.DRY_RUN_MODE;

    if (dry_run_mode == true) {
        g_dry_run = true;
    }


    // ===== Menu Renderer =====

    std::vector<std::pair<int, std::string>> menuItems = {
        {1, "List Drives"},                             {2, "Format Drive"},                                {3, "Encrypt/Decrypt USB Drives"},
        {4, "Resize Drive"},                            {5, "Check Drive Health"},                          {6, "Analyze Disk Space"},
        {7, "Overwrite Drive Data"},                    {8, "View Drive Metadata"},                         {9, "View Info/help"},
        {10, "Universal Disk tool (ISO/mount/...)"},    {11, "Forensic Analysis/Disk Image (experimental)"},
        {12, "Log viewer"},                             {13, "Clone a Drive"},                              {14, "Config Editor"}, 
        {15, "Fingerprint Drive"},                      {16, "Updater"},
        {0, "Exit"}
    };

    // *func for no_color 
    using menu_renderer = int(*)(const std::vector<std::pair<int, std::string>> &menuItems);
    menu_renderer menu_render_strategy = nullptr;

    if (g_no_color == true) {
        menu_render_strategy = MainMenuIO::noColorTuiMenu;
    } else {
        menu_render_strategy = MainMenuIO::colorTuiMenu;
    } 

    term.initiateTerminosInput();

    bool running = true;
    while (running == true) {
        int selected = menu_render_strategy(menuItems);

        int menuinput = menuItems[selected].first;
        switch (static_cast<MenuOptionsMain>(menuinput)) {

            case LISTDRIVES: {
                ListDrivesUtil::listDrives(false);
                std::cout << BOLD << "\nPress '1' to return, '2' for advanced listing, or '3' to exit:\n" << RESET;
                int menuques2;
                std::cin >> menuques2;
                if (menuques2 == 1) continue;
                else if (menuques2 == 2) listpartisions();
                else if (menuques2 == 3) running = false;
                break;
            }

            // 2. Format Drive
            case FORMATDRIVE:           { if (!checkRoot()) {menuQues(running);} else {formatDrive(); menuQues(running);} break; }

            // 3. En-Decrypt Drive
            case ENCRYPTDECRYPTDRIVE:   { if (!checkRoot()) {menuQues(running);} else {USBEnDeCryptionUtils::mainUsbEnDecryption(); menuQues(running);} break; } 

            // 4. Resize Drive
            case RESIZEDRIVE:           { if (!checkRoot()) {menuQues(running);} else {resizeDrive(); menuQues(running);} break; }

            // 5. Check Drive health
            case CHECKDRIVEHEALTH:      { if (!checkRoot()) {menuQues(running);} else {checkDriveHealth(); menuQues(running);} break; }

            // 6. Analyze Drive Space
            case ANALYZEDISKSPACE:      { analyzeDiskSpace(); menuQues(running); break; }

            // 7. Overwrite Data
            case OVERWRITEDRIVEDATA:    { if (!checkRoot()) {menuQues(running);} else {overwriteDriveData(); menuQues(running);} break; }

            // 8. View Metadata
            case VIEWMETADATA:          { if (!checkRootMetadata()) {menuQues(running);} else {MetadataReader::mainReader(); menuQues(running);} break; }

            // 9. View Info
            case VIEWINFO:              { Info(); menuQues(running); break; }

            // 10. Mount Unmount Drive
            case MOUNTUNMOUNT:          { if (!checkRoot()) {menuQues(running);} else {MountUtility::mainMountUtil(); menuQues(running);} break; }

            // 11. Forensics
            case FORENSIC:              { if (!checkRoot()) {menuQues(running);} else {ForensicAnalysis::mainForensic(); menuQues(running);} break; }

            // 12. Log viewer
            case LOGVIEW:               { logViewer(); menuQues(running); break; }

            // 13. Clone Drive
            case CLONEDRIVE:            { if (!checkRoot()) {menuQues(running);} else {Clone::mainClone(); menuQues(running);} break; }

            // 14. Config edtior
            case CONFIG:                { ConfigValueHandeling::configEditor(); menuQues(running); break; }
            
            // 15. Fingerprint Drive
            case FINGERPRINT:           { if (!checkRootMetadata()) {menuQues(running);} else {DriveFingerprinting::fingerprinting_main(); menuQues(running);} break; }

            // 16. Updater
            case UPDATER:               { LDMUpdater::updaterMain(VERSION); menuQues(running); break; }

            // 0. Exit
            case EXITPROGRAM:           { running = false; break; }

            default: {
                std::cerr << RED << "[Error] Invalid selection\n" << RESET;
                break;
            }
        }
    }
    
    std::cout << LEAVETERMINALSCREEN;
    return 0;
}
