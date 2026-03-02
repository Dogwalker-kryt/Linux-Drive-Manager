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

#ifndef DMGR_UPDATER
#define DMGR_UPDATER

#include <iostream>
#include <sstream>
#include <string>
#include "DmgrLib.h"
#include "exec_cmd.h"

class DmgrUpdater {
private:
    /**
     * @brief Internal struct to hold version components
     */
    struct Version_int {
        int major_realease = 0;
        int major = 0;
        int minor = 0;
        int patch = 0;
    };

    /**
     * @brief Parses version string (e.g., "v0.9.17.15") into version components
     * @param v version string from #define VERSION
     */
    static Version_int parseVersionVals(const std::string &v) {
        Version_int ver;
        std::string ver_str = v;

        if (!ver_str.empty() && (ver_str[0] == 'v' || ver_str[0] == 'V')) {
            ver_str.erase(0, 1);
        }

        size_t dash_pos = ver_str.find('-');
        if (dash_pos != std::string::npos) {
            ver_str = ver_str.substr(0, dash_pos);
        }

        std::stringstream ss(ver_str);
        char dot;
        ss >> ver.major_realease >> dot
            >> ver.major >> dot 
            >> ver.minor >> dot 
            >> ver.patch;
        return ver;
    }

    /**
     * @brief Compares the local version with the latest released one on github
     * @param version_local the local version in DriveMgr_experi.cpp
     * @param version_github the remote version on github
     */
    static int __comparing_versions(const Version_int &version_local, const Version_int &version_github) {
        if (version_local.major_realease != version_github.major_realease) {
            return (version_local.major_realease < version_github.major_realease) ? -1 : 1;
        }
        if (version_local.major != version_github.major) {
            return (version_local.major < version_github.major) ? -1 : 1;
        }
        if (version_local.minor != version_github.minor) {
            return (version_local.minor < version_github.minor) ? -1 : 1;
        }
        if (version_local.patch != version_github.patch) {
            return (version_local.patch < version_github.patch) ? -1 : 1;
        }
        return 0;
    }

    /**
     * @brief Fetches the latest release version from GitHub API
     * @return version string (e.g., "v0.9.17.15") or empty string on error
     */
    static std::string getVersionGithub() {
        std::string cmd = "curl -s https://api.github.com/repos/Dogwalker-kryt/Linux-Drive-Manager/releases/latest";
        auto res = EXEC_QUIET_SUDO(cmd);
        std::string json = res.output;

        size_t pos = json.find("\"tag_name\"");
        if (pos == std::string::npos) {
            return "";
        }

        pos = json.find(":", pos);
        pos = json.find("\"", pos) + 1;
        size_t end = json.find("\"", pos);
        return json.substr(pos, end - pos);
    }

public:
    /**
     * @brief Main updater function - checks GitHub for new releases
     * @param LOCAL_VERSION the current version from #define VERSION
     */
    static void updaterMain(const std::string &LOCAL_VERSION) {
        std::cout << "Initializing Updater...\n";
        std::cout << YELLOW << "\n[INFO] " << RESET << "Make sure you are connected to the internet\n";

        std::string dev_suffix;

        if (LOCAL_VERSION.find("-dev") != std::string::npos || LOCAL_VERSION.find("_dev") != std::string::npos) {
            dev_suffix = "Local version contains " + BOLD + "'-dev'." + RESET + "Local version is a developer/custom/other release build\n";    
        }


        Version_int local_version = parseVersionVals(LOCAL_VERSION);
        std::string remote_version_str = getVersionGithub();
        Version_int remote_version = parseVersionVals(remote_version_str);

        int cmp = __comparing_versions(local_version, remote_version);

        if (cmp < 0) {
            std::cout << "\n[UPDATE] New version available: " << remote_version_str << "\n";
        } else if (cmp > 0) {
            std::cout << "\n[INFO] Local version is newer (probably dev build or custom build)\n";

            if (!dev_suffix.empty()) {
                std::cout << dev_suffix;
            }

        } else {
            std::cout << "\n[INFO] You are up to date.\n";

            if (!dev_suffix.empty()) {
                std::cout << dev_suffix;
            }
        }
    }
};

#endif
