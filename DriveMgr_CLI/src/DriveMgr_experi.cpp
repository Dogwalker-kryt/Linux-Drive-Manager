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
// v0.9.25.63

// C++ libraries
#include <regex>
#include <cstdint>

// openssl includes
#include <openssl/sha.h>

// custom includes
#include "../include/DmgrLib.h"
#include "../include/LDM_updater.h"
#include "../include/ListDrivesUtil.hpp"
#include "../include/tests.hpp"
#include "../include/MenuIO.hpp"
#include "../include/globals.h"
#include "../include/StringUtils.hpp"
#include "../include/dmgr_runtime_error.hpp"
#include "../include/TermiosIO.h"


// ==================== definitions ====================

// === Version ===
#define VERSION std::string("v0.9.25.63")

// ========== Partition Management ========== 

class PartitionsUtils {
    private:
        // 1
        static bool resizePartition(const std::string& device, uint64_t newSizeMB) {
            try {

                const std::string cmd = "parted --script " + device + " resizepart 1 " + std::to_string(newSizeMB) + "MB";
                                 
                const auto res = EXEC(cmd);
                return res.success;

            } catch (const std::exception&) {

                ERR(ErrorCode::ProcessFailure, "Failed to resize partition");
                LOG_ERROR("Failed to resize partition");
                return false;

            }
        }

        // 2
        static bool movePartition(const std::string& device, int partNum, uint64_t startSectorMB) {
            try {

                const std::string cmd = "parted --script " + device + " move " + std::to_string(partNum) + " " + std::to_string(startSectorMB) + "MB";
                                 
                const auto res = EXEC_SUDO(cmd);
                return res.success;

            } catch (const std::exception&) {

                ERR(ErrorCode::ProcessFailure, "Failed to move partition");
                LOG_ERROR("Failed to move partition");
                return false;

            }
        }

        // 3
        static bool changePartitionType(const std::string& device, int partNum, const std::string& newType) {
            try {

                const std::string backupCmd = "sfdisk -d " + device + " > " + device + "_backup.sf";
                EXEC_QUIET(backupCmd);

                const std::string cmd = "echo 'type=" + newType + "' | sfdisk --part-type " + device + " " + std::to_string(partNum);
                                 
                const auto res = EXEC(cmd); 
                const std::string output = res.output;
                return output.find("error") == std::string::npos;

            } catch (const std::exception&) {

                ERR(ErrorCode::ProcessFailure, "Failed to change partition type");
                LOG_ERROR("Failed to change partition type");
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
                LOG_ERROR("Expected input is a integer");
                return;

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
                LOG_ERROR("You cannot assign a number <=0 to a uint64 interger");
                return;

            }

            if (newSize == 0) {

                ERR(ErrorCode::OutOfRange, "newSize cannot be equal to 0; Expected a number greater then 0 for uint64_t integer");
                LOG_ERROR("newSize cannot be equal to 0");
                return;

            }

            askForConfirmation("[Warning] Resizing partitions can lead to data loss.\nAre you sure? ");

            if (PartitionsUtils::resizePartition(partitions[partNum-1], newSize)) {

                std::cout << "Partition resized successfully!\n";

            } else {

                ERR(ErrorCode::ProcessFailure, "Failed to resize partition");
                LOG_ERROR("Failed to resize partition");
                return;

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
                LOG_ERROR("Expected input is a integer");
                return;

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
                LOG_ERROR("You cannot assign a number <=0 to a uint64 interger");
                return;

            }

            if (startPos == 0) {

                ERR(ErrorCode::OutOfRange, "startPos cannot be equal to 0; Expected a number greater then 0 for uint64_t integer");
                LOG_ERROR("startPos cannot be equal to 0");
                return;

            }

            askForConfirmation("[Warning] Moving partitions can lead to data loss.\nAre you sure? ");

            if (movePartition(partitions[partNum-1], partNum, startPos)) {

                std::cout << "Partition moved successfully!\n";

            } else {

                ERR(ErrorCode::ProcessFailure, "Failed to move partition");
                LOG_ERROR("Failed to move partition");
                return;

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

            auto typeNum = InputValidation::getInt(1, 4);
            if (!typeNum.has_value()) return;

            std::string newType;

            

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

                    ERR(ErrorCode::ProcessFailure, "Failed to change partition type");
                    LOG_ERROR("Failed to change partition type");
                    return;

                }
            }
        }
};

void listpartisions() { 
    const std::string drive_name = ListDrivesUtil::listDrives(true); 

    std::cout << "\nPartitions of drive " << drive_name << ":\n";

    const std::string cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -n -p " + drive_name; 
    const auto res = EXEC_QUIET(cmd); 

    if (!res.success) {

        ERR(ErrorCode::ProcessFailure, "lsblk failed");
        LOG_ERROR("lsblk failed");

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

    const auto choice = InputValidation::getInt(1, 4);
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
    const std::string drive_name = ListDrivesUtil::listDrives(true); 

    std::cout  << CYAN << "\n------ Disk Information ------\n" << RESET;

    std::string disk_cmd = "lsblk -b -o NAME,SIZE,TYPE,MOUNTPOINT -n -p " + drive_name;
    const auto disk_cmd_res = EXEC(disk_cmd); 

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
        const auto df_res = EXEC(df_cmd); 
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

        auto confirmation = InputValidation::getChar({'y', 'n'});
        if (!confirmation.has_value()) return false;
        
        if (confirmation != 'y') {

            std::cout << RED << "[INFO] " << RESET << "Formatting cancelled by user\n";
            LOG_INFO("Formatting cancelled for drive: " + drive);
            return false;

        }

        return true;
    }

public:
    static void format_drive(const std::string& drive_to_format, const std::string& label = "", const std::string& fs_type = "ext4") {
        if (!label.empty()) {

            if (label.length() > 16) {

                ERR(ErrorCode::OutOfRange, "Label too long (max 16 chars)");
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

                ERR(ErrorCode::ProcessFailure, "Failed to format drive: " + drive_to_format);
                LOG_ERROR("Format failed: " + drive_to_format);
                return;

            }
            
            std::cout << res.output << "\n";
            std::cout << GREEN << "[INFO] Drive formatted successfully\n" << RESET;

            LOG_INFO("Drive formatted: " + drive_to_format);
            
        } catch(const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Exception during formatting: " + std::string(e.what()));
            LOG_ERROR("Format exception: " + std::string(e.what()));
            return;

        }
    }
    
    // Wrapper functions for backward compatibility
    static void formatDriveBasic(const std::string& drive) { format_drive(drive); }
    static void formatDriveWithLabel(const std::string& drive, const std::string& label) { format_drive(drive, label); }
    static void formatDriveWithLabelAndFS(const std::string& drive, const std::string& label, const std::string& fs) { format_drive(drive, label, fs); }
};

void formatDrive() {
    std::cout << "INFO: Standard formatting will automaticlly use ext4 filesystem\n";
    int fdinput = GenericMenuIO::noColorTuiMenu("Format", {{1, "Format drive"}, {2, "Format drive with label"}, {3, "Format drive with label and filesystem"}, {0, "Exit"} });
    
    switch (fdinput) {
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

        case 0:
            break;

        default: {

            ERR(ErrorCode::OutOfRange, "Invalid formatting option selected");
            return;

        }
    }
}


// ========== Drive Health Check ==========

int checkDriveHealth() {
    std::cout << "[Check Drive health]\n";
    const std::string driveHealth_name = ListDrivesUtil::listDrives(true);

    try {

        const std::string health_cmd = "smartctl -H " + driveHealth_name;
        const auto res = EXEC_QUIET_SUDO(health_cmd);
        const std::string health_output = StrUtils::removeFirstLines(res.output, 3); 
        std::cout << health_output;

    } catch(const std::exception& e) {

        std::string error = e.what();
        LOG_ERROR(error);
        ERR(ErrorCode::ProcessFailure, e.what());

    }

   return 0;
}


// ========== Drive Resizing ==========

void resizeDrive() {
    std::cout << "\n[Resize Drive]\n";
    const std::string driveName = ListDrivesUtil::listDrives(true);

    std::cout << "Enter new size in GB for drive " << driveName << ":\n";

    const auto new_size = InputValidation::getUint();
    if (!new_size.has_value()) return;

    if (new_size == 0) {
        ERR(ErrorCode::OutOfRange, "new_size cannot be equal to 0; Expected a number greater then 0 for uint integer");
        return;
    }

    std::cout << "Resizing drive " << driveName << " to " << std::to_string(new_size.value_or(0)) << " GB...\n";

    try {

        const std::string resize_cmd = "sudo parted --script " + driveName +  " resizepart 1 " + std::to_string(new_size.value_or(0)) + "GB";
        const auto res = EXEC(resize_cmd); std::string resize_output = res.output;

        std::cout << resize_output << "\n";

        if (resize_output.find("error") != std::string::npos) {

            ERR(ErrorCode::ProcessFailure, "Failed to resize drive: " + driveName);
            LOG_ERROR("Failed to resize drive: " + driveName );

        } else {

            std::cout << GREEN << "Drive resized successfully\n" << RESET;
            LOG_SUCCESS("Drive resized successfully: " + driveName );

        }

    } catch (const std::exception& e) {

        ERR(ErrorCode::ProcessFailure, "Exception during resize: " + std::string(e.what()));
        LOG_ERROR("Exception during resize: " + std::string(e.what()));

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
            LOG_ERROR("lsblk failed to succed");
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
        std::string tran = StrUtils::toLowerString(meta[Metadata::TRAN] = extract("TRAN"));

        if (meta[Metadata::TYPE] != "disk") {

            ERR(ErrorCode::InvalidDevice, "Drive is not a Disk " + drive_name);
            LOG_ERROR("Drive is not a disk " + drive_name);
            return std::nullopt;

        }

        if (meta[Metadata::VENDOR] == "N/A" || meta[Metadata::VENDOR] == "ATA") {

            ERR(ErrorCode::InvalidDevice, "Drive is an internal disk " + drive_name + "; Expected USB Drive");
            LOG_ERROR("Drive is an internal Disk " + drive_name);
            return std::nullopt;
                        
        }

        if (tran != "usb") {

            ERR(ErrorCode::InvalidDevice, "Drive is not an USB Device " + drive_name + "; Expected USB Drive");
            LOG_ERROR("Drive is an internal Disk " + drive_name);
            return std::nullopt;
                        
        }

        return drive_name;
    }

    static bool confirmationKeyInput() {
        std::cout << "\nTo proceed with anything you need to retype the following confirmation key:\n";

        const std::string confirmation_key = confirmationKeyGenerator();
        std::cout << "\n" << confirmation_key << "\n";

        std::cout << "\nretype the key:\n";

        std::string user_retyped_key;
        std::getline(std::cin, user_retyped_key);

        if (user_retyped_key != confirmation_key) {

            std::cout << YELLOW << "[INFO] " << RESET << "The key you retyped doesnt match the original key\n" << "Process Aborted due to invalid input\n";
            LOG_INFO("The retyped key doesnt match the original key; Process Aborted due to invalid input");

            std::cout << "Do you want to retry? (y/N)\n";

            const auto confirm_if_retry = InputValidation::getChar({'y', 'n'});
            if (!confirm_if_retry.has_value()) return false;

            if (confirm_if_retry == 'n') {

                std::cout << YELLOW << "[INFO] " << RESET << "User aborted retry\n";
                LOG_INFO("Key retry was aborted by the user");
                return false;

            }

            std::string confirm_key2 = confirmationKeyGenerator();

            std::cout << "\n[last chance] Retype the following confirmation key:\n";
            std::cout << "\n" << confirm_key2 << "\n";

            std::string confirm_key2_input;
            std::getline(std::cin, confirm_key2_input);

            if (confirm_key2_input != confirm_key2) {

                std::cout << YELLOW << "[INFO] " << RESET << "The key you retyped doesnt match the original key\n" << "Process Aborted due to invalid input\n";
                LOG_INFO("The retyped key doesnt match the original key; Process Aborted due to invalid input");
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

        if (!passphrase.empty()) {

            ERR(ErrorCode::InvalidInput, "The passphrase you entered is emtpy; Expecting non empty string");
            LOG_ERROR("The passphrase you entered is emtpy");
            return;

        }

        std::cout << "\nRetype your Passphrase you just entered:\n";
        std::getline(std::cin, passphrase_retype);
    
        if (passphrase.empty() || passphrase_retype.empty()) {

            ERR(ErrorCode::InvalidInput, "The passphrase you entered is emtpy; Expecting non empty string");
            LOG_ERROR("The passphrase you entered is emtpy");
            return;

        }

        if (passphrase != passphrase_retype) {

            ERR(ErrorCode::InvalidInput, "The passphrases you entered doesnt match; Expecting similar passphrase input");
            LOG_ERROR("The passphrases you entered doesnt match: p2:'" + passphrase_retype + "'");
            return;

        }
    
        std::ofstream tmpfile("/tmp/LDM_tmp_dump.txt");
        tmpfile << passphrase_retype;
        tmpfile.close();

        // pass passphrases to crypsetup
        const std::string cryptsetup_cmd = "cryptsetup luksFormat " + drive_name + " --key-file=/tmp/LDM_tmp_dump.txt -q && shred /tmp/LDM_tmp_dump.txt";
        const auto cryptsetup_res = EXEC_SUDO(cryptsetup_cmd);
    
        if (!cryptsetup_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to execute cryptsetup on: " + drive_name);
            LOG_ERROR("Failed to execute cryptsetup on: " + drive_name);
            return;

        }

        // open encrypted device
        std::cout << "[INFO] open encrypted device...\n";
        const std::string mapper_name = "enc_usb";
        const std::string mapper_path = "/dev/mapper/" + mapper_name;

        const std::string cryptsetup_open_cmd = "echo \"" + passphrase + "\" | cryptsetup open " + drive_name + " " + mapper_name + " --key-file=-";
        const auto cryptsetup_open_res = EXEC_SUDO(cryptsetup_open_cmd);

        if (!cryptsetup_open_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to open drive crypsetup on: " + drive_name);
            LOG_ERROR("Failed to open drive with cryptsetup on: " + drive_name);
            return;

        }       

        // When you need a diffrent FS then change it here
        const std::string mkfs_ext4_cmd = "mkfs.ext4 " + mapper_path;
        const auto mkfs_res = EXEC_SUDO(mkfs_ext4_cmd);

        if (!mkfs_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to make FS on " + mapper_name);
            LOG_ERROR("Failed to make FS on " + mapper_name);
            return;            

        }

        // mount encrypted device
        std::cout << "[INFO] mounting encrypted device...\n";
        const std::string mount_cmd = "mount " + mapper_path + " /media/" + mapper_name;
        const auto mk_mountpoint_res = EXEC_SUDO("mkdir -p /media/" + mapper_name);
        const auto mount_res = EXEC_SUDO(mount_cmd);

        if (!mk_mountpoint_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to create mountpoint");
            LOG_ERROR("Failed to create mountpoint");
            return;     

        }

        if (!mount_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to mount " + mapper_name);
            LOG_ERROR("Failed to mount " + mapper_name);
            return;            

        }    

        // close 
        std::cout << "[INFO] closing encrypted device...\n";
        const auto unmount_cryptsetup_res = EXEC_SUDO("umount /media/" + mapper_name);

        if (!unmount_cryptsetup_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to unmount /media/" + mapper_name);
            LOG_ERROR("Failed to unmount /media/" + mapper_name);
            return;

        }

        const std::string close_cryptsetup_cmd = "cryptsetup close " + mapper_name;
        const auto close_cryptsetup_res = EXEC_SUDO(close_cryptsetup_cmd);

        if (!close_cryptsetup_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to close " + mapper_name);
            LOG_ERROR("Failed to close " + mapper_name);
            return;            

        }

        std::fill(passphrase.begin(), passphrase.end(), '\0');
        std::fill(passphrase_retype.begin(), passphrase_retype.end(), '\0');

        std::cout << GREEN << "\n[SUCCESS] " << RESET << "Encryption completed successfully\n";
        LOG_SUCCESS("Encryption completed successfully");
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
            LOG_ERROR("Passphrase empty");
            return;

        }

        // mapper name
        const std::string mapper_name = "enc_usb";
        const std::string mapper_path = "/dev/mapper/" + mapper_name;

        // cryptsetup open
        const std::string cryptsetup_open_cmd = "echo \"" + passphrase + "\" | cryptsetup open " + drive_name + " " + mapper_name + " --key-file=-";

        const auto cryptsetup_open_res = EXEC_SUDO(cryptsetup_open_cmd);

        if (!cryptsetup_open_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to open encrypted device with cryptsetup");
            LOG_ERROR("Failed to open encrypted device with cryptsetup");
            return;

        }

        // mount
        const auto mk_mountpoint_res = EXEC_SUDO("mkdir -p /media/" + mapper_name);

        if (!mk_mountpoint_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to create mountpoint");
            LOG_ERROR("Failed to create mountpoint");
            return;

        }

        const std::string mount_cmd = "mount " + mapper_path + " /media/" + mapper_name;
        const auto mount_res = EXEC_SUDO(mount_cmd);

        if (!mount_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to mount decrypted device");
            LOG_ERROR("Failed to mount decrypted device");
            return;

        }

        std::cout << GREEN << "\n[SUCCESS] " << RESET  << "USB successfully unlocked and mounted at /media/" << mapper_name << "\n";
        LOG_SUCCESS("USB successfully unlocked and mounted");

        std::cout << YELLOW << "[INFO] " << RESET << "Press ENTER when you are done using the USB to unmount and lock it again.\n";
        std::cin.get();

        // unmount
        const auto unmount_res = EXEC_SUDO("umount /media/" + mapper_name);

        if (!unmount_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to unmount decrypted device");
            LOG_ERROR("Failed to unmount decrypted device");
            return;

        }

        // close
        const std::string close_cmd = "cryptsetup close " + mapper_name;
        const auto close_res = EXEC_SUDO(close_cmd);

        if (!close_res.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to close mapper device");
            LOG_ERROR("Failed to close mapper device");
            return;

        }

        // wipe passphrase from memory
        std::fill(passphrase.begin(), passphrase.end(), '\0');

        std::cout << GREEN << "\n[SUCCESS] " << RESET << "USB successfully locked and unmounted.\n";
        LOG_SUCCESS("USB successfully locked and unmounted");
    }

    static void cryptionAltMenu(const std::string &drive_name) {
        std::cout << "\nDo you want to Encrypt or Decrypt your USB? (e/d):\n";

        const auto e_or_d = InputValidation::getChar({'e', 'd'});
        if (!e_or_d.has_value()) return;

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
        std::cout << "[En/Decrypt USB]\n";

        const std::string drive_name = ListDrivesUtil::listDrives(true);

        const auto val_drive_name = isValidDrive(drive_name);
    
        if (!val_drive_name.has_value()) {

            ERR(ErrorCode::InvalidDevice, "'" + drive_name + "' Couldnt get validated; Expected USB Drive");
            LOG_ERROR("'" + drive_name + "' Couldnt get validated");
            return;

        }
    
        std::cout << YELLOW << "\n[Warning] " << RESET << "Are you sure you want to en- or decrypt: '" << drive_name << "' ? (y/N)\n";
    
        const auto confirmation = InputValidation::getChar({'y', 'n'});
        if (!confirmation.has_value()) return;

        const bool is_confirm_key_true = confirmationKeyInput();

        if (!is_confirm_key_true) {

            LOG_INFO("ConfirmKeyInput is false, aborting operation");
            return;

        }

        if (confirmation == 'y') {

            std::cout << "\nProceeding...\n";
            cryptionAltMenu(drive_name);

        } else if (confirmation == 'n') {

            std::cout << YELLOW << "[INFO] " << RESET << "En- Decryption with '" << drive_name << "' was aborted by the user\n";
            LOG_INFO("En- Decryption with '" + drive_name + "' was aborted by the user");
            return;

        }
    }
};


// ========== Drive Data Overwriting ==========
// Tried my best to make this as safe and readable and maintainable as possible. v0.9.12.92

void overwriteDriveData() { 
    std::cout << BOLD << "\n[Drive Data Overwriting]" << RESET << "\n";
    const std::string drive_to_operate_on = ListDrivesUtil::listDrives(true);

    std::cout << YELLOW << "[WARNING]" << RESET << " Are you sure you want to overwrite all data on " << BOLD << drive_to_operate_on << RESET << "? This action cannot be undone! (y/n)\n";
        
    const auto confirm = InputValidation::getChar({'y', 'n'});
    if (!confirm.has_value()) return;

    if (confirm != 'y') {

        std::cout << BOLD << "[Overwriting aborted]" << RESET << " The Overwriting process of " << drive_to_operate_on << " was interupted by user\n";
        LOG_INFO("Overwriting process aborted by user for drive: " + drive_to_operate_on);
        return;

    }

    std::cout << "\nTo be sure you want to overwrite the data on " << BOLD << drive_to_operate_on << RESET << " you need to enter the following safety key\n";

    const std::string conf_key = confirmationKeyGenerator();
    LOG_INFO("Confirmation key generated for overwriting drive: " + drive_to_operate_on);

    std::cout << "\n" << conf_key << "\n";
    std::cout << "\nEnter the confirmation key:\n";

    std::string user_input;
    std::getline(std::cin, user_input);

    if (user_input != conf_key) {

        std::cout << BOLD << "[INFO]" << RESET << " The confirmationkey was incorrect, the overwriting process has been interupted\n";
        LOG_INFO("Incorrect confirmation key entered, overwriting process aborted for drive: " + drive_to_operate_on);
        return;

    }

    std::cout << YELLOW << "\n[Process]" << RESET << " Proceeding with overwriting all data on: " << drive_to_operate_on << "\n";
    std::cout << " \n";

    try {
        auto res_urandom = EXEC_SUDO("dd if=/dev/urandom of=" + drive_to_operate_on + " bs=8M status=progress && sync"); 
        auto res_zero = EXEC_SUDO("dd if=/dev/zero of=" + drive_to_operate_on + " bs=8M status=progress && sync"); 
            
        if (!res_urandom.success && !res_zero.success) {

            ERR(ErrorCode::ProcessFailure, "Failed to overwrite the drive: " + drive_to_operate_on);
            LOG_ERROR("Overwriting failed to complete for drive: " + drive_to_operate_on);
            return;

        } else if (!res_urandom.success || !res_zero.success) {

            std::cout << YELLOW << "[Warning]" << RESET << " One of the overwriting operations failed, but the drive may have been partially overwritten. Please check the output and try again if necessary.\n";
            LOG_WARNING("One of the overwriting operations failed for drive: " + drive_to_operate_on);
            return;
        } 

        std::cout << GREEN << "[Success]" << RESET << " Overwriting completed successfully for drive: " << drive_to_operate_on << "\n";
        LOG_SUCCESS("Overwriting completed successfully for drive: " + drive_to_operate_on);
        return;

    } catch (const std::exception& e) {

        LOG_ERROR("Failed during overwriting process for drive: " + drive_to_operate_on + " Reason: " + e.what());
        ERR(ErrorCode::ProcessFailure, "Exception during overwriting process: " + std::string(e.what()));
        return;

    }
}


// ========== Drive Metadata Reader ==========
// getMetadata refactor

class MetadataReader {
private:
    static std::optional<DriveMetadataStruct::DriveMetadata> getMetadata(const std::string& drive) {
        DriveMetadataStruct::DriveMetadata metadata;
        // -P (Pairs) is the key here. It output KEY="VALUE"
        const std::string cmd = "lsblk -o NAME,SIZE,MODEL,SERIAL,TYPE,MOUNTPOINT,VENDOR,FSTYPE,UUID -P -p " + drive; 

        const auto res = EXEC_QUIET(cmd);

        if (!res.success || res.output.empty()) { 

            ERR(ErrorCode::ProcessFailure, "The lsblk failed to deliver data");
            LOG_ERROR("lsblk failed to deliver data");
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

    static void displayMetadata(const DriveMetadataStruct::DriveMetadata& metadata) {
        std::cout << "\n-------- Drive Metadata --------\n";

        auto printAttr = [&](const std::string& attr, const std::string& value) {
            std::cout << "| " << attr << ": " << (value.empty() ? "N/A" : value) << "\n";
        };

        printAttr("Name", metadata.name.value_or("[ERROR] No Data available"));
        printAttr("Size", metadata.size.value_or("[ERROR] No Data available"));
        printAttr("Model", metadata.model.value_or("[ERROR] No Data available"));
        printAttr("Serial", metadata.serial.value_or("[ERROR] No Data available"));
        printAttr("Type", metadata.type.value_or("N/A"));
        printAttr("Mountpoint", metadata.mountpoint.value_or("Not mounted"));
        printAttr("Vendor", metadata.vendor.value_or("N/A"));
        printAttr("Filesystem", metadata.fstype.value_or("N/A"));
        printAttr("UUID", metadata.uuid.value_or("N/A"));

        if (metadata.type == "disk") {

            std::cout << "\n┌-─-─-─- SMART Data -─-─-─-─\n";
            
            const std::string smartCmd = "smartctl -i " + *metadata.name;
            const auto res = EXEC_QUIET_SUDO(smartCmd); 

            if (!res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to retrieve SMART data for " + *metadata.name);
                LOG_ERROR("Failed to retrieve SMART data for " + *metadata.name);
                return;

            }

            const std::string smartOutput = StrUtils::removeFirstLines(res.output, 4);

            if (!smartOutput.empty()) {

                std::cout << smartOutput;

            } else {

                ERR(ErrorCode::CorruptedData, "Failed to retrieve SMART data for " + *metadata.name);
                LOG_ERROR("Failed to retrieve SMART data for " + *metadata.name);

            }
        }   
        std::cout << "└─  - -─ --- ─ - -─-  - ──- ──- ───────────────────\n";         
    } 
    
public:
    static void mainReader() {
        std::cout << "\n[Drive Metadata]\n";
        const std::string driveName = ListDrivesUtil::listDrives(true);
        auto metadata = getMetadata(driveName);

        try {

            if (!metadata.has_value()) {

                ERR(ErrorCode::ProcessFailure, "Failed to read metadata for drive: " + driveName);
                LOG_ERROR("Failed to read metadata for drive: " + driveName);
                return;
                
            }

            displayMetadata(*metadata);

            LOG_SUCCESS("Successfully read metadata for drive: " + driveName);

        } catch (const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Failed to read drive metadata: " + std::string(e.what()));
            LOG_ERROR("Failed to read drive metadata: " + std::string(e.what()));
            return;

        }

        DriveMetadataStruct::clearMetadata(*metadata);
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
            LOG_ERROR("ISO file does not exist: " + iso_path);
            return false;

        }

        constexpr std::streamoff iso_magic_offset = 32769; // 16 * 2048 + 1

        if (fs::file_size(iso_path) < iso_magic_offset + 5) {

            ERR(ErrorCode::InvalidInput, "ISO file too small to contain valid metadata: " + iso_path);
            LOG_ERROR("ISO file too small to contain valid metadata: " + iso_path);
            return false;

        }

        std::ifstream iso_file(iso_path, std::ios::binary);

        if (!iso_file) {

            ERR(ErrorCode::IOError, "Cannot open ISO file: " + iso_path);
            LOG_ERROR("Cannot open ISO file: " + iso_path);
            iso_file.close();
            return false;

        }

        iso_file.seekg(iso_magic_offset);

        char buffer[6] = {};
         
        if (!iso_file.read(buffer, 5)) {

            ERR(ErrorCode::IOError, "Failed to read ISO metadata from file: " + iso_path);
            LOG_ERROR(" Failed to read ISO metadata: " + iso_path);
            iso_file.close();
            return false;

        }

        // ISO9660 magic signature
        if (std::strncmp(buffer, "CD001", 5) != 0) {

            ERR(ErrorCode::InvalidInput, "Invalid ISO signature in file: " + iso_path);
            LOG_ERROR("Invalid ISO signature in file: " + iso_path);
            iso_file.close();
            return false;

        }

        iso_file.close();
        return true;
    }

    static void BurnISOToStorageDevice() {
        std::cout << "\nChoose the drive you want to burn the ISO/IMG file on:\n";
        try {
            const std::string drive_name = ListDrivesUtil::listDrives(true);

            std::cout << "Enter the path to the ISO/IMG file you want to burn on " << drive_name << ":\n";

            std::string iso_path;
            std::getline(std::cin >> std::ws, iso_path);

            if (size_t pos = iso_path.find_first_of("-'&|<>;\""); pos != std::string::npos) {

                ERR(ErrorCode::InvalidInput, "Invalid characters in ISO path: " + iso_path);
                LOG_ERROR("Invalid characters in ISO path\n");
                return;

            }

            if (!IsoFileMetadataChecker(iso_path)) {

                ERR(ErrorCode::InvalidInput, "Invalid ISO file: " + iso_path);
                LOG_ERROR("Invalid ISO file: " + iso_path);
                return;

            }

            std::cout << "Are you sure you want to burn " << iso_path << " to " << drive_name << "? (y/n)\n";

            const auto confirmation = InputValidation::getChar({'y', 'n'});
            if (!confirmation.has_value()) return;

            if (confirmation != 'y') {

                std::cout << YELLOW << "[INFO] Operation cancelled\n" << RESET;
                LOG_INFO("Burn operation cancelled by user");
                return;

            }

            const std::string confirmation_key = confirmationKeyGenerator();
            std::cout << "\nEnter the confirmation key to proceed:\n";
            std::cout << confirmation_key << "\n";
                        
            std::string user_key_input;
            std::getline(std::cin, user_key_input);

            if (user_key_input != confirmation_key) {

                ERR(ErrorCode::InvalidInput, "Incorrect confirmation key.");
                LOG_ERROR("Incorrect confirmation key ");
                return;

            }

            std::cout << "\n" << YELLOW << "[PROCESS] Burning ISO to device...\n" << RESET;

            const auto unmount_res = EXEC_SUDO("umount " + drive_name + "* 2>/dev/null || true");
            
            if (!unmount_res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to unmount drive: " + drive_name);
                LOG_ERROR("dd burn failed for drive: " + drive_name);
                return;  

            }

            const auto res = EXEC_SUDO("dd if=" + iso_path + " of=" + drive_name + " bs=4M status=progress && sync"); 

            if (!res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to burn ISO: " + res.output);
                LOG_ERROR("dd burn failed for drive: " + drive_name);
                return;

            }

            std::cout << GREEN << "[SUCCESS] Successfully burned " << iso_path << " to " << drive_name << "\n" << RESET;

            LOG_SUCCESS("Successfully burned ISO to drive: " + drive_name);

        } catch (const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Burn operation failed: " + std::string(e.what()));
            LOG_ERROR("BurnISOToStorageDevice() exception: " + std::string(e.what()));
            return;

        }
    }

    /**
     * @brief wrapps the orgirnal unmount() and mount() funcs to gether in one
     * @param mount_or_unmount type in mount, you will get the mount function, type in unmount you will get the unmount fukntion
     */
    static void choose_mount_unmount(const std::string &mount_or_unmount) {
        if (mount_or_unmount == "mount") {

            std::cout << "\n[Mounting]\n";
            std::cout << "Enter the drive you want to mount:\n";
            const std::string drive_name = ListDrivesUtil::listDrives(true);

            std::cout << "\nEnter the name for the drive under the name its mounted under '/mnt/':\n";

            std::string mount_name;
            std::getline(std::cin, mount_name);

            if (mount_name.size() > 64) {

                ERR(ErrorCode::InvalidInput, "Mount name too long; Expected max length of 64 characters");
                LOG_ERROR("Mount name too long: " + mount_name);
                return;

            }

            if (mount_name.empty()) {

                ERR(ErrorCode::InvalidInput, "Mount name cannot be empty");
                LOG_ERROR("Mount name cannot be empty");
                return;

            }

            if (mount_name.find_first_of("-'&|<>;\"") != std::string::npos) {

                ERR(ErrorCode::InvalidInput, "Mount name contains invalid characters");
                LOG_ERROR("Mount name contains invalid characters");
                return;

            }

            const std::string mount_cmd = "mount " + drive_name + " /mnt/" + mount_name;
            const auto mount_res = EXEC_SUDO(mount_cmd);
             
            if (!mount_res.success) {

                ERR(ErrorCode::ProcessFailure, "Couldnt mount '" + drive_name + "' at '/mnt/" + mount_name);
                LOG_ERROR("Couldnt mount '" + drive_name + "' at '/mnt/" + mount_name);
                return;

            }

        } else if (mount_or_unmount == "unmount") {

            std::cout <<  "\n[Unmounting]\n";
            std::cout << "Enter the name, under wich the target drive is mounted (it maby under '/mnt/' or '/media/<user>/'):\n";

            std::string unmount_name;
            std::getline(std::cin, unmount_name);

            if(!fileExists(unmount_name)) {

                ERR(ErrorCode::FileNotFound, "The path you entered doesnt exist: " + unmount_name);
                LOG_ERROR("The path you entered doesnt exist: " + unmount_name);
                return;

            }

            if (unmount_name.find("/mnt/") != std::string::npos && unmount_name.find("/media/") != std::string::npos) {

                ERR(ErrorCode::InvalidDevice, "The path you entered doesnt contain /mnt/ or /media/, what results in an failing unmount operation");
                LOG_ERROR("The path you entered doesnt contain /mnt/ or /media/, what results in an failing unmount operation");
                return;

            }

            const std::string unmount_cmd = "umount " + unmount_name;
            const auto unmount_res = EXEC_SUDO(unmount_cmd);
             
            if (!unmount_res.success) {

                ERR(ErrorCode::ProcessFailure, "Couldnt unmount " + unmount_name);
                LOG_ERROR("Couldnt unmount " + unmount_name);
                return;

            }
        }

        return;
    }

    static void Restore_USB_Drive() {
        std::cout << "\nChoose the USB/Drive you want to restore (overwrite with empty filesystem and partition table):\n";
        const std::string restore_device_name = ListDrivesUtil::listDrives(true);

        try {

            std::cout << "Are you sure you want to overwrite/clean the ISO/Disk_Image from: " << restore_device_name << " ? [y/n]\n";
            
            const auto restore_confirm = InputValidation::getChar({'y', 'n'});
            if (!restore_confirm.has_value()) return;

            if (restore_confirm != 'y') {

                std::cout << CYAN << "[INFO] " << RESET << "Operation cancelled\n";
                LOG_INFO("restore usb operation cancelled");
                return;

            }

            EXEC_QUIET_SUDO("umount " + restore_device_name + "* 2>/dev/null || true");
            
            const auto wipefs_res = EXEC_SUDO("wipefs -a " + restore_device_name);

            if (!wipefs_res.success) {

                ERR(ErrorCode::ProcessFailure, "Failed to wipe device: " + restore_device_name);
                LOG_ERROR("Failed to wipe the filesystem of " + restore_device_name);
                return;

            }

            // Zero out start
            const auto dd_res = EXEC_SUDO("dd if=/dev/zero of=" + restore_device_name + " bs=1M count=10 status=progress && sync");

            if (!dd_res.success) {

                LOG_ERROR("Failed to overwrite the iso image on the usb");
                ERR(ErrorCode::ProcessFailure, "Failed to overwrite device: " + restore_device_name);
                return;

            }

            // Create partition table
            const auto parted_res = EXEC_SUDO("parted -s " + restore_device_name + " mklabel msdos mkpart primary 1MiB 100%");

            if (!parted_res.success) {

                LOG_ERROR("Failed while restoring USB: " + restore_device_name);
                ERR(ErrorCode::ProcessFailure, "Failed to create partition table on USB device: " + restore_device_name);
                return;

            }

            // Probe partitions
            const auto partprobe_res = EXEC_SUDO("partprobe " + restore_device_name);

            if (!partprobe_res.success) { 

                ERR(ErrorCode::ProcessFailure, "Couldnt partition the drive: " + restore_device_name);
                LOG_ERROR("Couldnt partition the drive: " + restore_device_name);
                return;

            }

            std::string partition_path = restore_device_name;

            if (!partition_path.empty() && std::isdigit(partition_path.back())) {

                partition_path += "p1"; 
            
            } else { 

                partition_path += "1";

            }

            const auto mkfs_res = EXEC_SUDO("mkfs.vfat -F32 " + partition_path);

            if (!mkfs_res.success) {

                LOG_ERROR("Failed while formatting USB: " + restore_device_name);
                ERR(ErrorCode::ProcessFailure, "Failed to format USB device with FS: " + restore_device_name);
                return;

            }

            std::cout << GREEN << "[Success] Your USB should now function as a normal FAT32 drive (partition: " << partition_path << ")\n" << RESET;
            LOG_SUCCESS("Restored USB device " + restore_device_name + " -> formatted " + partition_path);
            return;

        } catch (const std::exception& e) {

            ERR(ErrorCode::ProcessFailure, "Failed to initialize usb restore function: " + std::string(e.what()));
            LOG_ERROR("failed to initialize restore usb function");
            return;

        }
    }

    // ========== Menu that took that i made in 1:51 am in the morning ===========
    enum MenuOptions {
        Burniso = 1, MountDrive = 2, UnmountDrive = 3, RESTOREUSB = 4, Exit = 0
    };

    static std::vector<std::pair<int, std::string>> getMenuItems() {
        return {
            {Burniso, "Burn iso/img to storage device"},
            {MountDrive, "Mount storage device"},
            {UnmountDrive, "Unmount storage device"},
            {RESTOREUSB, "Restore usb from iso"},
            {Exit, "Return to main menu"}
        };
    }

public:
    static void mainMountUtil() {
        const int menu_input = GenericMenuIO::noColorTuiMenu("Mount/Unmount", getMenuItems()); 
        
        switch (menu_input) {
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
                LOG_INFO("Operation cancelled");
                return;
            }

            auto res = EXEC_SUDO("dd if=" + driveName + " of=" + imagePath + " bs=4M status=progress && sync");
            if (!res.success) {
                ERR(ErrorCode::ProcessFailure, "Failed to create disk image");
                LOG_ERROR("Failed to create disk image for drive: " + driveName);
                return;
            }

            std::cout << GREEN << "[Success] Disk image created at " << imagePath << "\n" << RESET;
            LOG_SUCCESS("Disk image created successfully for drive: " + driveName);
       
        } catch (const std::exception& e) {
            ERR(ErrorCode::ProcessFailure, "Failed to create Disk image: " + std::string(e.what()));
            LOG_ERROR("Failed to create disk image: " + std::string(e.what()));
            return;
        }
    }

    // recoverymain + side functions
    static void recovery() {
        std::vector<std::pair<int, std::string>> recovery_menu = {
            {1, "Files Recovery"},
            {2, "Partition Recovery"},
            {3, "System Recovery"},
            {0, "Return to main menu"}
        };

        int menu_choice = GenericMenuIO::noColorTuiMenu("Recovery", recovery_menu);

        switch (menu_choice) {
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

        auto depth = InputValidation::getInt({1, 2});
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
        auto save_dump = InputValidation::getChar({'y', 'n'});
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
                LOG_INFO("Partition-table dump saved: " + dumpPath + " for device: " + device);
           
            } else {

                std::cout << "[Warning] Could not write partition-table dump.\n";
                LOG_INFO("Failed to write partition-table dump for device: " + device);
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
            LOG_ERROR("Could not create system recovery script: " + scriptPath);
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
            LOG_INFO("User allowed DriveMgr to run system recovery helper script: " + scriptPath);
            
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

        std::vector<std::pair<int, std::string>> forensic_menu = {
            {Info, "Info about the Forensic Analysis tool"},
            {CreateDisktImage, "Create a disk image of a drive"},
            {ScanDrive, "Recover system/files/partitions..."},
            {Exit, "Return to main menu"}
        };

        int menu_choice = GenericMenuIO::noColorTuiMenu("Forensic Analysis", forensic_menu);

        switch (static_cast<ForensicMenuOptions>(menu_choice)) {
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
            
            const auto confirmation = InputValidation::getChar({'y', 'n'});
            if (!confirmation.has_value()) return;

            if (confirmation != 'y') {

                std::cout << "[Info] Operation cancelled\n";
                LOG_INFO("Operation cancelled");
                return;

            } else if (confirmation == 'y') {
                try {

                    const auto res = EXEC_SUDO("dd if=" + source + " of=" + target + " bs=5M status=progress && sync");

                    if (!res.success) {

                        LOG_ERROR("Failed to clone drive from " + source + " to " + target);
                        ERR(ErrorCode::ProcessFailure, "Failed to clone data from " + source + " to " + target);
                        return;

                    }

                    std::cout << GREEN << "[Success] Drive cloned from " << source << " to " << target << "\n" << RESET;
                    LOG_SUCCESS("Drive cloned successfully from " + source + " to " + target);
                
                } catch (const std::exception& e) {

                    ERR(ErrorCode::ProcessFailure, "Failed to clone drive: " + std::string(e.what()));
                    LOG_ERROR(std::string("Failed to clone drive from ") + source + " to " + target + e.what());
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
                LOG_ERROR("Target drive cannot be empty string");
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

                    LOG_ERROR("Source and target drives are the same");
                    dmgr_runtime_error("[ERROR] Source and target drives cannot be the same!");
                    return;

                } else {

                    CloneDrive(source_drive, val_target);
                    return;
                }

            } catch (std::exception& e) {

                ERR(ErrorCode::ProcessFailure, "An error occurred during the clone initializing process: " + std::string(e.what()));
                LOG_ERROR(std::string(e.what()));
                return; 

            }
        }
};


// ========== Log Viewer Utility ==========

void logViewer() {
    std::ifstream file(log_path);

    if (!file) {

        LOG_ERROR("Unable to read log file at " + log_path.string());
        ERR(ErrorCode::FileNotFound, "Unable to read log file at path: " + log_path.string());

        std::cout << "Please read the log file manually at: " << log_path.string() << "\n";
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
    
    const auto clear_loggs = InputValidation::getChar({'y', 'n'});
    if (!clear_loggs.has_value()) return;

    if (clear_loggs == 'y') {

        Logger::clearLoggs(log_path);

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

            if (g_config_src_flag == true) {

                config_path = g_config_src_path;

            }

            if (config_path.empty()) {

                ERR(ErrorCode::DataUnavailable, "Using default config values!");
                LOG_ERROR("config file is using defautl values, due to emtpy config");
                return cfg;

            }

            if (!std::filesystem::exists(config_path)) {

                ERR(ErrorCode::FileNotFound, "Config file not found at path: " + config_path.string() + ". Check if the config exists and is readable. Returning default config values.");
                LOG_ERROR("Config file not found at path: " + config_path.string());
                return cfg;

            }

            std::ifstream config_file(config_path);

            if (!config_file.is_open()) {

                LOG_ERROR("[Config_handler] Cannot open config file");
                ERR(ErrorCode::FileNotFound, "Cannot open config file at path: " + config_path.string() + ". Check if the config exists and is readable. Returning default config values.");
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
                key = StrUtils::trimWhiteSpace(key);
                value = StrUtils::trimWhiteSpace(value);

                if (key == "UI_MODE") cfg.UI_MODE = value;
                else if (key == "COMPILE_MODE") cfg.COMPILE_MODE = value;
                else if (key == "COLOR_THEME") cfg.THEME_COLOR_MODE = value;
                else if (key == "SELECTION_COLOR") cfg.SELECTION_COLOR_MODE = value;
                else if (key == "DRY_RUN_MODE") {
                    std::string v = StrUtils::toLowerString(value);
                    cfg.DRY_RUN_MODE = (v == "true");
                }
                else if (key == "ROOT_MODE") {
                    std::string v = StrUtils::toLowerString(value);
                    cfg.ROOT_MODE = (v == "true");
                };
            }
            return cfg;
        }

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
            
            const auto config_edit_confirm = InputValidation::getChar({'y', 'n'}); 
            if (!config_edit_confirm.has_value()) return; 

            if (config_edit_confirm != 'y') return;

            if (!std::filesystem::exists(lume_path)) {

                ERR(ErrorCode::FileNotFound, "Lume editor not found at: " + lume_path.string());
                LOG_ERROR("Lume editor missing at: " + lume_path.string());
                return;

            }

            if (!std::filesystem::exists(config_path)) {

                ERR(ErrorCode::FileNotFound, "Config file not found at: " + config_path.string());
                LOG_ERROR("Config file missing at: " + config_path.string());
                return;

            }

            const std::string cmd = "\"" + lume_path.string() + "\" \"" + config_path.string() + "\"";

            std::cout << LEAVETERMINALSCREEN << std::flush;
            term.restoreTerminal();

            system(cmd.c_str());

            std::cout << NEWTERMINALSCREEN << std::flush;

            term.enableRawMode();
            return;      
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
    static DriveMetadataStruct::DriveMetadata getMetadata(const std::string& drive) {
        DriveMetadataStruct::DriveMetadata metadata;
        const std::string cmd = "lsblk -o NAME,SIZE,MODEL,SERIAL,UUID -P -p " + drive; 

        const auto res = EXEC_QUIET(cmd);

        if (!res.success || res.output.empty()) { 

            ERR(ErrorCode::ProcessFailure, "The lsblk failed to deliver data");
            LOG_ERROR("lsblk failed to deliver data");
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

        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {

            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", hash[i]);
            fingerprint += hex;

        }
        
        if (fingerprint.empty()) {

            LOG_ERROR("Failed to generate fingerprint");
            ERR(ErrorCode::ProcessFailure, "Failed to generate fingerprint");
            return "";

        }

        return fingerprint;
    }


public:
    static void fingerprinting_main() {
        std::cout << "\n[Drive Fingerprinting]\n";
        const std::string drive_name_fingerprinting = ListDrivesUtil::listDrives(true);

        DriveMetadataStruct::DriveMetadata metadata = getMetadata(drive_name_fingerprinting);

        LOG_INFO("Retrieved metadata for drive: " + drive_name_fingerprinting);

        const std::string combined_metadata =
            *metadata.name + "|" +
            *metadata.size + "|" +
            *metadata.model + "|" +
            *metadata.serial + "|" +
            *metadata.uuid;

        const std::string fingerprint = fingerprinting(combined_metadata);

        LOG_INFO("Generated fingerprint for drive: " + drive_name_fingerprinting);

        std::cout << BOLD << "Fingerprint:\n" << RESET;
        std::cout << "\r\033[K\n";
        std::cout << fingerprint << "\n";

        DriveMetadataStruct::clearMetadata(metadata);
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

static void printUsage(const char* progname) {
    std::cout << "Usage: " << progname << " [options]\n";
    std::cout << BOLD << "Options:\n" << RESET 
              << "  --version, -v       Print program version\n"
              << "  --help, -h          Show this help and exit\n"
              << "  --dry-run, -n       Do not perform destructive operations\n"
              << "  --no-color, -c      Disable colors (may affect the main menu)\n"
              << "  --no-log, -nl       Disables all logging in the current session\n"
              << "  --debug, -d         Enables debug messages in current session and Test option\n"
              << "  --info, -i          Show program info\n"
              << "  --logs, -l          Show log file content\n"
              << "  --select <device>, -sd <device>         Pre select a drive you want to use\n"
              << "  --config-src <path>, -cfg-src <path>    Use a diffrent config source temporalily\n"
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
}


// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    std::cout << NEWTERMINALSCREEN;

    const std::map<std::string, std::function<void()>> cli_commands = {
        {"--list-drives", []()          { std::cout << LEAVETERMINALSCREEN; ListDrivesUtil::listDrives(false); }},
        {"--format-drive", []()         { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; formatDrive(); }},
        {"--encrypt-decrypt", []()      { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; USBEnDeCryptionUtils::mainUsbEnDecryption(); }}, 
        {"--resize-drive", []()         { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; resizeDrive(); }},
        {"--check-drive-health", []()   { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; checkDriveHealth(); }},
        {"--analyze-disk-space", []()   { term.enableTerminosInput_diableAltTerminal(); analyzeDiskSpace(); }},
        {"--overwrite-drive-data", []() { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; overwriteDriveData(); }},
        {"--view-metadata", []()        { term.enableTerminosInput_diableAltTerminal(); if (!checkRootMetadata()) return; MetadataReader::mainReader(); }},
        {"--forensics", []()            { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; ForensicAnalysis::mainForensic(); }},
        {"--clone-drive", []()          { term.enableTerminosInput_diableAltTerminal(); if (!checkRoot()) return; Clone::mainClone(); }},
        {"--fingerprint", []()          { term.enableTerminosInput_diableAltTerminal(); if (!checkRootMetadata()) return;  DriveFingerprinting::fingerprinting_main();  }}
    };

    for (int i = 1; i < argc; i++) {
        std::string a(argv[i]); 

        if (a == "--no-color" || a == "-c")                        { g_no_color = true; continue; }

        if (a == "--no-log" || a == "-nl")                         { g_no_log = true; continue; }

        if (a == "--help" || a == "-h")                            { std::cout << LEAVETERMINALSCREEN; printUsage(argv[0]); return 0; }

        if (a == "--version" || a == "-v")                         { std::cout << LEAVETERMINALSCREEN; std::cout << "DriveMgr CLI version: " << VERSION << "\n"; return 0; }
        
        if (a == "--debug" || a == "-d")                           { g_debug = true; continue; }

        if (a == "--logs" || a == "-l")                            { logViewer(); std::cout << LEAVETERMINALSCREEN; return 0; }

        if (a == "--dry-run" || a == "-n")                         { g_dry_run = true; continue; }

        if (a == "--info" || a == "-i")                            { std::cout << LEAVETERMINALSCREEN; Info(); return 0; }

        if (a == "--select" || a == "-sd")                         { 

            g_selected_drive_by_flag = true; 
            
            g_selected_drive = argv[i + 1];
            i++;
            
            if (!fileExists(g_selected_drive)) {

                ERR(ErrorCode::DeviceNotFound, "");
                LOG_ERROR("The device: '" + g_selected_drive + "' could not be found");
                break;

            }

            continue; 
        }

        if (a == "--config-src" || a == "-cfg-src")                {

            g_config_src_flag = true;

            g_config_src_path = argv[i + 1];
            i++;

            std::string val_config_src_path = filePathHandler(g_config_src_path);

            g_config_src_path = val_config_src_path;

            if (!fileExists(g_config_src_path)) {

                ERR(ErrorCode::FileNotFound, "Your custom config: '" + g_config_src_path + "coudnt be found");
                LOG_ERROR("The file: '" + g_config_src_path + "' could not be found");
                break;

            } 

            continue;
        }

        else {
            auto cmd = cli_commands.find(argv[i]);
            
            if (cmd != cli_commands.end()) {
                cmd->second();
                return 0;
            }
        }
    }


    ConfigValueHandeling::colorThemeHandler();

    ConfigValueHandeling::CONFIG_VALUES cfg = ConfigValueHandeling::configHandler();
    bool dry_run_mode = cfg.DRY_RUN_MODE;

    if (dry_run_mode == true) {
        g_dry_run = true;
    }


    // ===== Menu Renderer =====

    std::vector<std::pair<MenuOptionsMain, std::string>> menuItems = {
        {LISTDRIVES, "List Drives"},                            {FORMATDRIVE, "Format Drive"},                                  {ENCRYPTDECRYPTDRIVE, "Encrypt/Decrypt USB Drives"},
        {RESIZEDRIVE, "Resize Drive"},                          {CHECKDRIVEHEALTH, "Check Drive Health"},                       {ANALYZEDISKSPACE, "Analyze Disk Space"},
        {OVERWRITEDRIVEDATA, "Overwrite Drive Data"},           {VIEWMETADATA, "View Drive Metadata"},                          {VIEWINFO, "View Info/help"},
        {MOUNTUNMOUNT, "Universal Disk tool (ISO/mount/...)"},  {FORENSIC, "Forensic Analysis/Disk Image (experimental)"},      {LOGVIEW, "Log viewer"},                                
        {CLONEDRIVE, "Clone a Drive"},                          {CONFIG, "Config Editor"}, {FINGERPRINT, "Fingerprint Drive"},  {UPDATER, "Updater"},                                    
        {EXITPROGRAM, "Exit"}
    };

    if (g_debug || (VERSION.find("dev") != std::string::npos) == true) {

        menuItems.insert(menuItems.end() - 1, {TESTS, "Tests"});
    }


    // *func for no_color 
    using menu_renderer = int(*)(const std::vector<std::pair<MenuOptionsMain, std::string>> &menuItems);
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
                auto menuques2 = InputValidation::getInt(1, 3);
                
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

            // 17. Tests
            case TESTS: { 
                auto res = run_all_tests_internal(); 
                print_test_summary(res);
                
                menuQues(running); 
                break; 
            }

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
