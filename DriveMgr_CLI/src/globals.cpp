#include "../include/globals.h"

// === color related globals ===

bool g_no_color = false;

std::string g_THEME_COLOR = "default";

std::string g_SELECTION_COLOR = "default";


// === drive related globals ===

std::string g_selected_drive;

bool g_selected_drive_by_flag = false;

std::vector<std::string> g_last_drives;


// === config related globals ===


bool g_config_src_flag = false;

std::string g_config_src_path;


// === program state globals ===

bool g_dry_run = false;
bool g_no_log = false;
bool g_debug =  false;

std::filesystem::path dmgr_root = EnvSys::appRoot();
std::filesystem::path log_path = dmgr_root / "data" / "log.dat";
std::filesystem::path config_path = dmgr_root / "data" / "config.conf";
std::filesystem::path lume_path = dmgr_root / "bin" / "Lume" / "Lume";