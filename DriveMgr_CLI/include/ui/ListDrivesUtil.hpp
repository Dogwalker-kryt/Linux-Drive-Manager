#pragma once

#include "../cmd_exec/exec_cmd.h"
#include "../DmgrLib.h"
#include "../DiskLister.hpp"

// ========== TUI drive selection/listing ==========

class ListDrivesUtil {
    private:
        struct Row {
            std::string device, size, type, mount, fstype, status;
        };

        inline static std::vector<ListDrivesUtil::Row> rows{};
        
        struct DriveCache {
            size_t run_count_idx;
            std::vector<Row> rows;
        };

        inline static DriveCache drive_cache{};
        inline static DiskLister disk_lister{};

        /**
         * @brief This is the Tui menu logic from the listDrive function finaly put in its own function
         * @param drives use the std::vector<std::string> where you stored your fetched drives
         * @param rows use the std::verctor<Row> rows, that is only in this class avilable
         */
        static std::string tuiForListDrives(const std::vector<std::string> &drives, std::vector<ListDrivesUtil::Row> &rows);

        static void printDriveRow(int idx, const Row& r);
        
        static void printDriveHeader();
    public:
        /**
         * @brief Prints available drives to the terminal. TUI input can be turned on
         * @param input_mode if 'true' then the TUI selection enables and returns the selected drive when pressed enter
         * @returns selected drive name as string. TUI must be enabled for this to happen
         */
        static std::string listDrives(bool input_mode);

        static std::string listDaDrives(bool input_mode);
};
