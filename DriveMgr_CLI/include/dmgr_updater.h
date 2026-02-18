#ifndef DMGR_UPDATER
#define DMGR_UPDATER

#include <iostream>
#include <sstream>
#include <string>
#include "drivefunctions.h"
#include "exec_cmd.h"

class DmgrUpdater {
private:
    /**
     * @brief Internal struct to hold version components
     */
    struct Version_int {
        int major = 0;
        int minor = 0;
        int patch = 0;
    };

    /**
     * @brief Parses version string (e.g., "v0.9.17.15") into version components
     * @param v version string from #define VERSION
     */
    Version_int parseVersionVals(const std::string &v) {
        Version_int ver;
        std::string ver_str = v;

        if (!ver_str.empty() && (ver_str[0] == 'v' || ver_str[0] == 'V')) {
            ver_str.erase(0, 1);
        }

        std::stringstream ss(ver_str);
        char dot;
        ss >> ver.major >> dot >> ver.minor >> dot >> ver.patch;
        return ver;
    }

    /**
     * @brief Compares the local version with the latest released one on github
     * @param version_local the local version in DriveMgr_experi.cpp
     * @param version_github the remote version on github
     */
    static int __comparing_versions(const Version_int &version_local, const Version_int &version_github) {
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

        Version_int local_version = DmgrUpdater().parseVersionVals(LOCAL_VERSION);
        std::string remote_version_str = getVersionGithub();
        Version_int remote_version = DmgrUpdater().parseVersionVals(remote_version_str);

        int cmp = __comparing_versions(local_version, remote_version);

        if (cmp < 0) {
            std::cout << "\n[UPDATE] New version available: " << remote_version_str << "\n";
        } else if (cmp > 0) {
            std::cout << "\n[INFO] Local version is newer (probably dev build or custom build)\n";
        } else {
            std::cout << "\n[INFO] You are up to date.\n";
        }
    }
};

#endif