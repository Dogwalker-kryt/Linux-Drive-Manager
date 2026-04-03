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

// command_exec.h
#pragma once

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <sstream>

struct ExecResult {
    int exit_code{ -1 };
    std::string stdout_str;
    std::string stderr_str;
};

static std::string __drive_read_all_from_fd(int fd) {
    std::string out;
    constexpr size_t BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    ssize_t r;
    while ((r = read(fd, buf, BUF_SIZE)) > 0) {
        out.append(buf, static_cast<size_t>(r));
    }
    return out;
}

// Inline implementation so users only need to compile the main CLI file.
static ExecResult run_command(const std::string& cmd, int /*timeout_seconds*/ = 0) {
    ExecResult res;
    res.exit_code = -1;

    int outpipe[2];
    int errpipe[2];
    if (pipe(outpipe) != 0 || pipe(errpipe) != 0) {
        res.stderr_str = "pipe failed";
        return res;
    }

    pid_t pid = fork();
    if (pid < 0) {
        res.stderr_str = "fork failed";
        close(outpipe[0]); close(outpipe[1]);
        close(errpipe[0]); close(errpipe[1]);
        return res;
    }

    if (pid == 0) {
        // child
        dup2(outpipe[1], STDOUT_FILENO);
        dup2(errpipe[1], STDERR_FILENO);
        // close unused fds
        close(outpipe[0]); close(outpipe[1]);
        close(errpipe[0]); close(errpipe[1]);
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)NULL);
        _exit(127);
    }

    // parent
    close(outpipe[1]);
    close(errpipe[1]);

    // read streams (blocking until child closes them)
    res.stdout_str = __drive_read_all_from_fd(outpipe[0]);
    res.stderr_str = __drive_read_all_from_fd(errpipe[0]);

    close(outpipe[0]);
    close(errpipe[0]);

    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w == -1) {
        res.exit_code = -1;
    } else if (WIFEXITED(status)) {
        res.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        res.exit_code = 128 + WTERMSIG(status);
    } else {
        res.exit_code = -1;
    }
    return res;
}
