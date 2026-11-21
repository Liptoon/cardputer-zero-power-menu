#pragma once

#include <string>

namespace power {

enum class Action { None, Restart, Shutdown };

struct Result {
    bool ok = false;
    std::string detail;
};

/* Runs the requested system action. Never returns on a successful poweroff or
 * reboot; returns a failure Result when no command could be executed. */
Result run(Action action, bool dry_run);

const char * actionName(Action action);

}  // namespace power
