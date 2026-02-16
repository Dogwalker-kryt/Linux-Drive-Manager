#include "../include/command_exec.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <sys/select.h>
#include <sys/time.h>

static std::string read_all_from_fd(int fd) {
    std::string out;
    constexpr size_t BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    ssize_t r;
    while ((r = read(fd, buf, BUF_SIZE)) > 0) {
        out.append(buf, static_cast<size_t>(r));
    }
    return out;
}

ExecResult run_command(const std::string& cmd, int /*timeout_seconds*/) {
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
    res.stdout_str = read_all_from_fd(outpipe[0]);
    res.stderr_str = read_all_from_fd(errpipe[0]);

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
