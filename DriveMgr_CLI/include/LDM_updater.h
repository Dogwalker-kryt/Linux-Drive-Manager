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

#ifndef LDM_UPDATER
#define LDM_UPDATER

#include "DmgrLib.h"
#include "exec_cmd.h"


class LDMUpdater {
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
    static Version_int parseVersionVals(const std::string &v);

    /**
     * @brief Compares the local version with the latest released one on github
     * @param version_local the local version in DriveMgr_experi.cpp
     * @param version_github the remote version on github
     */
    static int comparing_versions(const Version_int &version_local, const Version_int &version_github);

    /**
     * @brief Fetches the latest release version from GitHub API
     * @return version string (e.g., "v0.9.17.15") or empty string on error
     */
    static std::string getVersionGithub();

public:
    /**
     * @brief Main updater function - checks GitHub for new releases
     * @param LOCAL_VERSION the current version from #define VERSION
     */
    static void updaterMain(const std::string &LOCAL_VERSION);
};

#endif