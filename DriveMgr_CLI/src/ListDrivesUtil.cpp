#include "../include/ListDrivesUtil.hpp"


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

std::string ListDrivesUtil::tuiForListDrives(const std::vector<std::string> &drives, std::vector<ListDrivesUtil::Row> &rows) {
    term.enableRawMode();

    int selected = 0;
    int total = drives.size();

    // Move cursor UP to the first drive row
    std::cout << "\033[" << total << "A";

    while (true) {
        std::cout << "\r"; // go to start of line
        for (int i = 0; i < total; i++) {

            // Arrow indicator
            if (i == selected) { 
                if (!g_no_color) std::cout << g_SELECTION_COLOR;
                std::cout << "> ";
                if (!g_no_color) std::cout << RESET;
            } else { 
                std::cout << "  "; 
            }

            // Highlight row
            if (i == selected && !g_no_color) std::cout << g_SELECTION_COLOR;

            std::cout << std::left
                << std::setw(3)  << i
                << std::setw(18) << rows[i].device
                << std::setw(10) << rows[i].size
                << std::setw(10) << rows[i].type
                << std::setw(15) << rows[i].mount
                << std::setw(10) << rows[i].fstype
                << rows[i].status;

            if (i == selected && !g_no_color) std::cout << RESET;

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


void ListDrivesUtil::printDriveRow(int idx, const Row& r) {
    std::cout << std::left
              << std::setw(3)  << idx
              << std::setw(18) << r.device
              << std::setw(10) << r.size
              << std::setw(10) << r.type
              << std::setw(15) << r.mount
              << std::setw(10) << r.fstype
              << r.status << "\n";
}

void ListDrivesUtil::printDriveHeader() {
    if (!g_no_color) std::cout << g_THEME_COLOR;
    std::cout << "\nAvailable Drives:";

    if (!g_no_color) std::cout << RESET;
    std::cout << "\n";

    std::cout << std::left << std::setw(5) << "#";
    if (!g_no_color) std::cout << BOLD;
    std::cout << std::setw(18) << "Device";

    if (!g_no_color) std::cout << RESET << BOLD;
    std::cout << std::setw(10) << "Size";

    if (!g_no_color) std::cout << RESET << BOLD;
    std::cout << std::setw(10) << "Type";

    if (!g_no_color) std::cout << RESET << BOLD;
    std::cout << std::setw(15) << "Mountpoint";

    if (!g_no_color) std::cout << RESET << BOLD;
    std::cout << std::setw(10) << "FSType";

    if (!g_no_color) std::cout << RESET;
    std::cout << "Status" << std::endl;

    if (!g_no_color) std::cout << g_THEME_COLOR;
    std::cout << std::string(90, '-') << "\n";
    if (!g_no_color) std::cout << RESET;
}

std::string ListDrivesUtil::listDrives(bool input_mode) {
    if (g_selected_drive_by_flag == true) {
        return g_selected_drive;
    }

    static std::vector<std::string> drives;

    if (drive_cache.run_count_idx > 0) {
        printDriveHeader();

        for (int i = 0; i < drive_cache.rows.size(); i++) {
            printDriveRow(i, drive_cache.rows[i]);
        }

        if (!input_mode) return "";

        g_selected_drive = tuiForListDrives(drives, drive_cache.rows);
        return g_selected_drive;
    }

    drives.clear();

    // Run lsblk using new CmdExec
    auto lsblk_res = EXEC_QUIET("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE -d -n -p");
    std::string lsblk = lsblk_res.output;
            
    drive_cache.run_count_idx = 1;

    printDriveHeader();

    // Parse lsblk output
    std::istringstream iss(lsblk);
    std::string line;
    int idx = 0;

    while (std::getline(iss, line)) {
        if (line.find("disk") == std::string::npos) continue;

        std::istringstream lss(line);
        ListDrivesUtil::Row r;

        lss >> r.device >> r.size >> r.type;

        std::string rest;
        std::getline(lss, rest);
        std::istringstream rss(rest);

        rss >> r.mount >> r.fstype;
        if (r.mount == "-") r.mount = "";
        if (r.fstype == "-") r.fstype = "";

        r.status = checkFilesystem(r.device, r.fstype);

        printDriveRow(idx, r);

        drives.push_back(r.device);
        rows.push_back(r);
        idx++;
    }

    if (drives.empty()) {
        ERR(ErrorCode::DeviceNotFound, "No drives found");
        LOG_ERROR("No drives found");
        return "";
    }

    // g_last_drives = drives;

    drive_cache.rows = rows;

    if (input_mode != true) {
        return "";
    }

    g_selected_drive = tuiForListDrives(drives, rows);
    return g_selected_drive;
}       