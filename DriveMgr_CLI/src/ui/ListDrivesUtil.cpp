#include "../ui/ListDrivesUtil.hpp"

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
                if (!Globals::g_no_color) std::cout << Globals::g_SELECTION_COLOR;
                std::cout << "> ";
                if (!Globals::g_no_color) std::cout << RESET;
            } else { 
                std::cout << "  "; 
            }

            // Highlight row
            if (i == selected && !Globals::g_no_color) std::cout << Globals::g_SELECTION_COLOR;

            std::cout << std::left
                << std::setw(3)  << i
                << std::setw(16) << rows[i].device
                << std::setw(10) << rows[i].size
                << std::setw(10) << rows[i].type
                << std::setw(15) << rows[i].mount
                << std::setw(10) << rows[i].fstype
                << rows[i].status;

            if (i == selected && !Globals::g_no_color) std::cout << RESET;

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
    if (!Globals::g_no_color) std::cout << Globals::g_THEME_COLOR;
    std::cout << "\nAvailable Drives:";

    // if (!Globals::g_no_color) std::cout << RESET;
    std::cout << "\n";

    std::cout << std::left << std::setw(5) << "#";
    // if (!Globals::g_no_color) std::cout << BOLD;
    std::cout << BOLD;
    std::cout << std::setw(16) << "Device";

    std::cout << std::setw(10) << "Size";

    std::cout << std::setw(10) << "Type";

    std::cout << std::setw(15) << "Mountpoint";

    std::cout << std::setw(10) << "FSType";

    std::cout << "Status" << std::endl;

    // if (!Globals::g_no_color) std::cout << Globals::g_THEME_COLOR;
    std::cout << std::string(90, '-') << "\n";
    std::cout << RESET;
}

std::string ListDrivesUtil::listDrives(bool input_mode) {
    if (Globals::g_selected_drive_by_flag == true) {
        return Globals::g_selected_drive;
    }

    static std::vector<std::string> drives;

    if (drive_cache.run_count_idx > 0) {
        printDriveHeader();

        for (int i = 0; i < drive_cache.rows.size(); i++) {
            printDriveRow(i, drive_cache.rows[i]);
        }

        if (!input_mode) return "";

        Globals::g_selected_drive = tuiForListDrives(drives, drive_cache.rows);
        return Globals::g_selected_drive;
    }

    drives.clear();

    // Use DiskLister to fetch drives
    disk_lister.refresh();
    auto disk_info_rows = disk_lister.getPhysicalDisksInfoAsRows();
            
    drive_cache.run_count_idx = 1;

    printDriveHeader();

    // Convert DiskLister::Row to ListDrivesUtil::Row
    int idx = 1;
    for (const auto& disk_row : disk_info_rows) {
        ListDrivesUtil::Row r;
        r.device = disk_row.device;
        r.size = disk_row.size;
        r.type = disk_row.type;
        r.mount = disk_row.mount;
        r.fstype = disk_row.fstype;
        r.status = disk_row.status;

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

    drive_cache.rows = rows;

    if (input_mode != true) {
        return "";
    }

    Globals::g_selected_drive = tuiForListDrives(drives, rows);
    return Globals::g_selected_drive;
}       