#include "power_action.h"

#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace power {

namespace {

using Command = std::vector<std::string>;

const std::vector<Command> kRestartCommands = {
    {"systemctl", "reboot"},
    {"reboot"},
    {"shutdown", "-r", "now"},
};

const std::vector<Command> kShutdownCommands = {
    {"systemctl", "poweroff"},
    {"poweroff"},
    {"shutdown", "-h", "now"},
};

std::string join(const Command & command) {
    std::string line;
    for (const auto & part : command) {
        if (!line.empty()) {
            line += ' ';
        }
        line += part;
    }
    return line;
}

bool execute(const Command & command, std::string * error) {
    std::vector<char *> argv;
    argv.reserve(command.size() + 1);
    for (const auto & part : command) {
        argv.push_back(const_cast<char *>(part.c_str()));
    }
    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        *error = std::string("fork failed: ") + std::strerror(errno);
        return false;
    }
    if (pid == 0) {
        execvp(argv[0], argv.data());
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            *error = std::string("waitpid failed: ") + std::strerror(errno);
            return false;
        }
    }

    if (WIFEXITED(status)) {
        const int code = WEXITSTATUS(status);
        if (code == 0) {
            return true;
        }
        if (code == 127) {
            *error = "command not available";
            return false;
        }
        *error = "exit code " + std::to_string(code);
        return false;
    }
    if (WIFSIGNALED(status)) {
        *error = "terminated by signal " + std::to_string(WTERMSIG(status));
        return false;
    }

    *error = "unknown failure";
    return false;
}

}  // namespace

const char * actionName(Action action) {
    switch (action) {
        case Action::Restart:
            return "restart";
        case Action::Shutdown:
            return "shutdown";
        case Action::None:
        default:
            return "none";
    }
}

Result run(Action action, bool dry_run) {
    const std::vector<Command> & candidates =
        action == Action::Restart ? kRestartCommands : kShutdownCommands;
    if (action == Action::None || candidates.empty()) {
        return {false, "no action requested"};
    }

    if (dry_run) {
        return {true, "dry run: would run " + join(candidates.front())};
    }

    std::string last_error;
    for (const auto & command : candidates) {
        std::string error;
        if (execute(command, &error)) {
            return {true, "ran " + join(command)};
        }
        last_error = join(command) + ": " + error;
    }

    return {false, last_error};
}

}  // namespace power
