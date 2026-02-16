#ifndef EXEC_CMD_H
#define EXEC_CMD_H

#include "drivefunctions.h"

// ==================== Command Execution Abstraction ====================

enum class ExecMode {
    NORMAL,      // Regular execution
    DRY_RUN,     // Show what would run
    QUIET        // No output to console, only logging
};

// Using existing ExecResult from command_exec.h
// Extended version with convenience members
struct CmdExecResult {
    bool success;
    std::string output;
    int exit_code;
};

class CmdExec {
private:
    static inline bool check_error(const std::string& output, const std::string& cmd) {
        if (output.find("error") != std::string::npos || output.find("failed") != std::string::npos || output.find("ERROR") != std::string::npos) {
            Logger::log("[EXEC] Command failed: " + cmd, g_no_log);
            return false;
        }
        return true;
    }

public:
    // Main command executor
    static inline CmdExecResult run(const std::string& cmd, bool use_sudo = false, ExecMode mode = ExecMode::NORMAL) {
        CmdExecResult result{false, "", -1};
        
        std::string final_cmd = cmd;
        if (use_sudo) final_cmd = "sudo " + cmd;
        
        // Dry-run mode
        if (g_dry_run || mode == ExecMode::DRY_RUN) {
            std::cout << YELLOW << "[DRY-RUN] Would execute: " << final_cmd << RESET << "\n";
            Logger::log("[DRY-RUN] " + final_cmd, g_no_log);
            result.success = true;
            return result;
        }
        
        // Execute via existing terminal functions
        result.output = Terminalexec::execTerminalv2(final_cmd.c_str());
        result.success = check_error(result.output, cmd);
        
        if (mode == ExecMode::QUIET) {
            Logger::log("[EXEC] " + cmd + " -> " + (result.success ? "OK" : "FAILED"), g_no_log);
        } else {
            if (!result.output.empty()) std::cout << result.output << "\n";
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
            throw std::runtime_error("Command failed: " + cmd + "\n" + res.output);
        }
        return res;
    }
};

// Quick helpers for common patterns
#define EXEC(cmd)              CmdExec::run(cmd)
#define EXEC_SUDO(cmd)         CmdExec::run_sudo(cmd)
#define EXEC_QUIET(cmd)        CmdExec::run_quiet(cmd, false)
#define EXEC_QUIET_SUDO(cmd)   CmdExec::run_quiet(cmd, true)
#define EXEC_OR_THROW(cmd)     CmdExec::run_or_throw(cmd)

#endif // EXEC_CMD_H