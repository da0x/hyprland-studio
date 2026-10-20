// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "foundation/file-descriptor.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <sys/types.h>

namespace desktop {

    // The user's own live processes whose name (/proc/<pid>/comm) is `name`.
    [[nodiscard]] std::vector<pid_t> findProcesses(std::string_view name, const std::filesystem::path& processRoot = "/proc");

    // Starts `program` the way Hyprland starts one: in a session of its own, with no
    // terminal, and not as the studio's child, so it keeps running after the studio exits.
    [[nodiscard]] std::expected<void, std::string> startDetached(const std::filesystem::path& program);

    // Asks the process to exit, as closing it would.
    [[nodiscard]] std::expected<void, std::string> stopProcess(pid_t process);

    // A descriptor that becomes readable when the process exits.
    [[nodiscard]] std::expected<foundation::FileDescriptor, std::string> exitOf(pid_t process);

}
