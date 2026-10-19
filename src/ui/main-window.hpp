// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace ui {

    struct Options {
        // Show the toolkit's debug output, which is hidden otherwise.
        bool verbose = false;
    };

    // Opens the main window and runs the event loop until it closes.
    // Returns the process exit code.
    [[nodiscard]] int run(const Options& options);

}
