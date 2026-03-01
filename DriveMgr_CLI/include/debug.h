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

#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>

#define RED2 "\033[31m"
#define RESET2 "\033[0m"

enum class ErrorCode {
    PermissionDenied,
    DeviceNotFound,
    IOError,
    InvalidDevice,
    DeviceBusy,
    InvalidInput,
    OutOfRange,
    ProcessFailure,
    FileNotFound,
    CorruptedData,
    DataUnavailable,
    Unknown
};

inline const char* errorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::PermissionDenied: return "Permission denied";
        case ErrorCode::DeviceNotFound: return "Device not found";
        case ErrorCode::IOError: return "I/O Error";
        case ErrorCode::InvalidDevice: return "Invalid device";
        case ErrorCode::DeviceBusy: return "Device busy";
        case ErrorCode::InvalidInput: return "Invalid input";
        case ErrorCode::OutOfRange: return "Value out of range";
        case ErrorCode::ProcessFailure: return "Process failed to execute/load";
        case ErrorCode::FileNotFound: return "File not found";
        case ErrorCode::CorruptedData: return "Data is corrupted";
        case ErrorCode::DataUnavailable: return "Data is unavailable";
        default: return "Unknown Error";
    }
}

inline int printError(ErrorCode code, const std::string& extra_msg, const char* file, int line, const char* func) {
    std::cerr << RED2
              << "[ERROR] " << errorMessage(code)
              << " (" << file << ":" << line << ", " << func << ")"
              << " - " << extra_msg
              << RESET2 << "\n";
    return 1;
}


#define ERR(code, extra_msg) printError(code, extra_msg, __FILE__, __LINE__, __func__)


/**
 * @brief debug message for eysier debugging; added in v0.14.96
 * @param message contains the message
 * @param is_active if true then debug message will be printed
 */
int debug_msg(const std::string& message, bool is_active);



#endif 
