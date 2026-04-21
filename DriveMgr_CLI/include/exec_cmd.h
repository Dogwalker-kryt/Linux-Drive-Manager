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
#ifndef EXEC_CMD_H
#define EXEC_CMD_H

#include "DmgrLib.h"
#include "command_exec.h"
#include "Spinner.hpp"
#include <atomic>

// ==================== Command Execution Abstraction ====================

enum class ExecMode {
    NORMAL,      // Regular execution
    DRY_RUN,     // Show what would run
    QUIET,       // No output to console, only logging
    PROGRESS     // Show spinner
};

// Extended result type for high-level usage
struct CmdExecResult {
    bool success;          // true if exit_code == 0
    std::string output;    // combined stdout + stderr
    int exit_code;         // raw exit code from the process
};

class CmdExec {
public:
    static inline CmdExecResult run(const std::string& cmd, bool use_sudo = false, ExecMode mode = ExecMode::NORMAL) {
        CmdExecResult result{false, "", -1};

        std::string final_cmd = cmd;
        if (use_sudo) final_cmd = "sudo " + cmd;

        if (g_dry_run || mode == ExecMode::DRY_RUN) {

            std::cout << YELLOW << "[DRY-RUN] Would execute: " << final_cmd << RESET << "\n";
            LOG_DRYRUN(final_cmd);

            result.success = true;
            result.exit_code = 0;
            return result;

        }

        std::atomic<bool> b_done = false;
        std::thread spinner;

        if (mode == ExecMode::PROGRESS) {
            spinner = std::thread([&]() {
                SPINNER(b_done);
            });
        }

        // Execute via low-level runner
        ExecResult r = run_command(final_cmd);
        b_done = true;
        
        // Spinner
        if (spinner.joinable()) {
            spinner.join();
        }
        
        // Fill high-level result
        result.exit_code = r.exit_code;
        result.success   = (r.exit_code == 0);

        // You can choose: only stdout, or stdout+stderr.
        // For drive ops, having both is usually better:
        result.output = r.stdout_str;
        if (!r.stderr_str.empty()) {

            if (!result.output.empty())
                result.output += "\n";

            result.output += r.stderr_str;
        }

        // Logging / console behavior
        if (mode == ExecMode::QUIET) {

            LOG_EXEC(cmd + " -> " + (result.success ? "OK" : "FAILED"));

        } else {

            if (!result.output.empty())
                std::cout << result.output << "\n";

        }
        return result;
    }

    // Convenience overloads
    static inline CmdExecResult run_sudo(const std::string& cmd, ExecMode mode = ExecMode::NORMAL) {
        return run(cmd, true, mode);
    }

    static inline CmdExecResult run_quiet(const std::string& cmd, bool use_sudo = false) {
        return run(cmd, use_sudo, ExecMode::QUIET);
    }

    // Check and throw on failure
    static inline CmdExecResult run_or_throw(const std::string& cmd, bool use_sudo = false) {
        CmdExecResult res = run(cmd, use_sudo);
        if (!res.success) {

            std::cout << LEAVETERMINALSCREEN;
            throw std::runtime_error("Command failed: " + cmd + "\nExit code: " + std::to_string(res.exit_code) + "\nOutput:\n" + res.output);

        }
        return res;
    }

    static inline CmdExecResult run_spinner(const std::string &cmd) {
        return run(cmd, false, ExecMode::PROGRESS);
    }

    static inline CmdExecResult run_sudo_spinner(const std::string &cmd) {
        return run(cmd, true, ExecMode::PROGRESS);
    }
};

// Quick helpers for common patterns
/** 
 * @brief runs ``` cmd ``` command
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC(cmd);
 * 
 * or
 *
 * EXEC(cmd)
 */
#define EXEC(cmd)              CmdExec::run(cmd)

/** 
 * @brief runs ``` cmd ``` command as Sudo user
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC_SUDO(cmd);
 */
#define EXEC_SUDO(cmd)         CmdExec::run_sudo(cmd)

/** 
 * @brief runs ``` cmd ``` command with no text ouput printed to the terminal
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC_QUIET(cmd);
 */
#define EXEC_QUIET(cmd)        CmdExec::run_quiet(cmd, false)

/** 
 * @brief runs ``` cmd ``` command as Sudo with no text ouput printed to the terminal
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC_QUIET_SUDO(cmd);
 */
#define EXEC_QUIET_SUDO(cmd)   CmdExec::run_quiet(cmd, true)

/** 
 * @brief runs ``` cmd ``` command, if commands fails throws std::runtime_error
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC_OR_THROW(cmd);
 */
#define EXEC_OR_THROW(cmd)     CmdExec::run_or_throw(cmd)

/** 
 * @brief runs ``` cmd ``` command with spinner
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC_SPINNER(cmd);
 */
#define EXEC_SPINNER(cmd)      CmdExec::run_spinner(cmd)

/** 
 * @brief runs ``` cmd ``` command as Sudo with spinner
 * @param cmd as std::string 
 * @returns res.output, res.success, res.exit_code
 * @note 
 * auto res = EXEC_SPINNER(cmd);
 */
#define EXEC_SUDO_SPINNER(cmd) CmdExec::run_sudo_spinner(cmd)

#endif // EXEC_CMD_H
