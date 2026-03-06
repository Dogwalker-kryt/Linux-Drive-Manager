#include <iostream>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <pwd.h>
#define CONFIG_FILE_PATH "/.local/share/DriveMgr/data/config.conf"


struct CONFIG_VALUES {
    std::string UI_MODE = "CLI";
    std::string COMPILE_MODE = "StatBin";
    bool ROOT_MODE = false;
    bool DRY_RUN_MODE = false;
};


std::string filePathHandler(const std::string &file_path) {
    const char* sudo_user = getenv("SUDO_USER");
    const char* user_env = getenv("USER");
    const char* username = sudo_user ? sudo_user : user_env;

    if (!username) {
        std::cerr << "[ERROR] Could not determine username.\n";
       
        return "";
    }

    struct passwd* pw = getpwnam(username);

    if (!pw) {
        std::cerr << "[ERROR] Could not get home directory for user: " << username << "\n";
        return "";
    }

    std::string homeDir = pw->pw_dir;
    std::string path = homeDir + file_path;
    
    return path;
}


std::unordered_map<std::string, std::string> Programm_locations = {
    {"CLI", "/.local/share/DriveMgr/bin/bin/DriveMgr_CLI"},
    {"GUI", "/.local/share/DriveMgr/bin/bin/DriveMgr_GUI"}
};


CONFIG_VALUES configHandler() {
    CONFIG_VALUES cfg{};

    std::string conf_file = filePathHandler(CONFIG_FILE_PATH);

    if (conf_file.empty()) {
        return cfg; // return defaults
    }

    std::ifstream config_file(conf_file);
    if (!config_file.is_open()) {
        std::cerr << "[Config_handler ERROR] Cannot open config file: " << conf_file << "\n";
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

        else if (key == "DRY_RUN_MODE") {
            std::string v = value;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            cfg.DRY_RUN_MODE = (v == "true");
        }

        else if (key == "ROOT_MODE") {
            std::string v = value;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            cfg.ROOT_MODE = (v == "true");
        }
    }

    return cfg;
}


void launch_program(const std::string &program_path,
                    bool root,
                    bool dry_run,
                    const std::string &program_flags = "") 
{
    std::string path = filePathHandler(program_path);
    std::string cmd = path;

    if (!program_flags.empty()) {
        cmd += " " + program_flags;
    }

    if (dry_run) {
        cmd += " --dry-run";
    }

    if (root) {
        cmd = "sudo " + cmd;
        system(cmd.c_str());
    } else {
        system(cmd.c_str());
    }
}


int main(int argc, char* argv[]) {

    CONFIG_VALUES cfg = configHandler();

    std::string flag;
    if (argc > 1) {
        flag = argv[1];
    }

    std::string selected_ui = cfg.UI_MODE;
    if (Programm_locations.find(selected_ui) == Programm_locations.end()) {
        selected_ui = "CLI"; // fallback
    }

    std::string path = Programm_locations[selected_ui];

    launch_program(path, cfg.ROOT_MODE, cfg.DRY_RUN_MODE, flag);
    return 0;
}
