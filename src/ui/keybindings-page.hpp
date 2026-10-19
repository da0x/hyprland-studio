// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/keybindings.hpp"
#include "application/session.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <expected>
#include <functional>
#include <string>

namespace ui {

    // Every keybinding Hyprland has right now, what each one does and where the config set
    // it, kept current by the session.
    class KeybindingsPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        KeybindingsPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        void showConnection(application::Connection connection, const std::string& reason);
        void showKeybindings(const std::expected<application::Keybindings, std::string>& keybindings);

      private:
        void                                                                 updateStatus();

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>         status;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>         luaNote;
        RowList                                                              list;

        application::Connection                                              connection = application::Connection::connecting;
        std::string                                                          connectionReason;
        std::string                                                          bindingsError;
        std::string                                                          configurationProblem;
        std::size_t                                                          count    = 0;
        std::size_t                                                          inactive = 0;
    };

}
