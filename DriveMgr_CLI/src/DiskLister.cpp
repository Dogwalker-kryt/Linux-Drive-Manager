#include "../include/DiskLister.hpp"

std::string DiskLister::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

std::string DiskLister::getMountPoint(const std::string& device) {
    std::ifstream mtab("/etc/mtab");
    if (!mtab.is_open()) return "";
    std::string line;
    while (std::getline(mtab, line)) {
        std::istringstream iss(line);
        std::string dev, mount, fstype;
        if (std::getline(iss, dev, ' ') &&
            std::getline(iss, mount, ' ') &&
            std::getline(iss, fstype, ' ')) {
            if (dev.find(device) != std::string::npos) {
                return mount;
            }
        }
    }
    return "";
}

std::string DiskLister::getFsType(const std::string& device) {
    std::ifstream mtab("/etc/mtab");
    if (!mtab.is_open()) return "";
    std::string line;
    while (std::getline(mtab, line)) {
        std::istringstream iss(line);
        std::string dev, mount, fstype;
        if (std::getline(iss, dev, ' ') &&
            std::getline(iss, mount, ' ') &&
            std::getline(iss, fstype, ' ')) {
            if (dev.find(device) != std::string::npos) {
                return fstype;
            }
        }
    }
    return "";
}

std::vector<DiskInfo> DiskLister::getPhysicalDisksInfo() {
    disks.clear();
    
    for (const auto& entry : fs::directory_iterator("/sys/block/")) {
        std::string dev = entry.path().filename().string();
        if (dev.find("loop") != std::string::npos || dev.find("ram") != std::string::npos) continue;

        DiskInfo info;
        info.device = "/dev/" + dev;

        // Size (in bytes, from /sys/block/<dev>/size)
        std::string size = readFile("/sys/block/" + dev + "/size");
        if (!size.empty()) {
            size.erase(std::remove_if(size.begin(), size.end(), ::isspace), size.end());
            unsigned long sectors = std::stoul(size);
            info.size = std::to_string(sectors * 512 / 1024 / 1024 / 1024) + "G";
        }

        // Type (from /sys/block/<dev>/device/type)
        info.type = readFile("/sys/block/" + dev + "/device/type");
        info.type.erase(std::remove_if(info.type.begin(), info.type.end(), ::isspace), info.type.end());
        if (info.type == "0") info.type = "disk";

        // Mount point and filesystem type (from /etc/mtab)
        info.mount = getMountPoint(info.device);
        info.fstype = getFsType(info.device);

        // Status (from /sys/block/<dev>/stat)
        std::string stat = readFile("/sys/block/" + dev + "/stat");
        info.status = stat.empty() ? "unknown" : "running";

        disks.push_back(info);
    }
    
    return disks;
}

std::vector<Row> DiskLister::getPhysicalDisksInfoAsRows() {
    std::vector<Row> rows;
    auto disk_list = getPhysicalDisksInfo();
    for (const auto& disk : disk_list) {
        rows.push_back({disk.device, disk.size, disk.type, disk.mount, disk.fstype, disk.status});
    }
    return rows;
}

const std::vector<DiskInfo>& DiskLister::getCachedDisks() const {
    return disks;
}

void DiskLister::refresh() {
    getPhysicalDisksInfo();
}
