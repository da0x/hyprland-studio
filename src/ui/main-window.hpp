// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace ui {

    // Opens the main window and runs the event loop until it closes.
    // Returns the process exit code.
    [[nodiscard]] int run();

}
