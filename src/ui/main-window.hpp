// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace ui {

    struct Options {
        // Show the toolkit's debug output, which is hidden otherwise.
        bool        verbose = false;
        // The page to open, by its title in the sidebar; the first page when empty.
        std::string page;
    };

    // How the window closed.
    struct Ending {
        int         code = 0;
        // The studio asked to start again, on this page, as after a theme change: the
        // toolkit reads its colours only when an app starts.
        bool        restart = false;
        std::string page;
    };

    // Opens the main window and runs the event loop until it closes.
    [[nodiscard]] Ending run(const Options& options);

}
