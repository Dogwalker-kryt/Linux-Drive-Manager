#pragma once

#include <string>
#include <vector>

// === color related globals ===

/** @brief Global variable to indicate if color output is disabled, set by --no-color flag */
extern bool g_no_color;

/** @brief Global variable to hold the theme color for the TUI, set by --theme-color flag or default */
extern std::string g_THEME_COLOR;

/** @brief Global variable to hold the selection color for the TUI, set by --selection-color flag or default */
extern std::string g_SELECTION_COLOR;


// === drive related globals ===

/** @brief Global variable to hold the currently selected drive, set by --drive flag or TUI selection */
extern std::string g_selected_drive;

/** @brief Global variable to indicate if the drive was selected by --drive flag (true) or TUI selection (false) */
extern bool g_selected_drive_by_flag;

/** @brief Global variable to hold the list of drives from the last drive scan, used for comparison in subsequent scans */
extern std::vector<std::string> g_last_drives;


// === config related globals ===

/** @brief Global variable to indicate if the configuration source flag is set */
extern bool g_config_src_flag;

/** @brief Global variable to hold the path to the configuration source file, set by --config-src flag */
extern std::string g_config_src_path;


// === program state globals ===

/**
 * @brief Global variable to indicate if the program is running in dry-run mode, where no actual commands are executed and only logged
 */
extern bool g_dry_run;
