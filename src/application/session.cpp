// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/session.hpp"

#include "application/loaded-config.hpp"
#include "ipc/request.hpp"

#include <algorithm>

namespace application {

    namespace {

        // How long to wait before connecting again after the event socket drops.
        constexpr std::chrono::seconds retryDelay{2};

    }

    Session::Session(EventLoop loop, Listeners listeners) : loop{std::move(loop)}, listeners{std::move(listeners)} {}

    Session::~Session() {
        if (events)
            loop.unwatch(events->descriptor());
        loop.unwatch(worker.descriptor());
    }

    void Session::start() {
        loop.watch(worker.descriptor(), [this] { worker.runFinished(); });

        auto found = ipc::currentInstance();
        if (!found) {
            listeners.connection(Connection::lost, found.error());
            return;
        }
        instance = std::move(*found);
        connect();
    }

    void Session::connect() {
        listeners.connection(Connection::connecting, {});

        auto opened = ipc::EventConnection::open(*instance);
        if (!opened) {
            lose(opened.error());
            return;
        }
        events = std::move(*opened);
        loop.watch(events->descriptor(), [this] { readEvents(); });

        // Subscribed first, then the snapshot, so no change can fall between the two.
        listeners.connection(Connection::live, {});
        refreshKeybindings();
    }

    void Session::lose(const std::string& reason) {
        if (events) {
            loop.unwatch(events->descriptor());
            events.reset();
        }
        listeners.connection(Connection::lost, reason);
        loop.after(retryDelay, [this] { connect(); });
    }

    void Session::readEvents() {
        auto drained = events->drain();
        if (!drained) {
            lose(drained.error());
            return;
        }

        // A reload replaces every binding, and no event reports a single binding changing.
        // Hyprland also reloads when a config file changes, so this re-reads the config too.
        if (std::ranges::any_of(*drained, [](const ipc::Event& event) { return event.name == "configreloaded"; }))
            refreshKeybindings();
    }

    void Session::refreshKeybindings() {
        worker.post([this, instance = *instance]() -> Worker::Continuation {
            const auto reply = ipc::request(instance, ipc::Query::bindings);
            auto       live  = reply ? ipc::parseBindings(*reply) : std::expected<std::vector<ipc::Binding>, std::string>{std::unexpect, reply.error()};

            std::expected<Keybindings, std::string> keybindings{std::unexpect, live ? std::string{} : live.error()};
            if (live) {
                const auto loaded    = findLoadedConfig(instance);
                const auto recording = loaded.config ? configuration::record(loaded.config->file) : std::expected<configuration::Recording, std::string>{std::unexpect, loaded.config.error()};
                keybindings          = joinKeybindings(std::move(*live), recording);
                keybindings->configurationDoubts = loaded.doubts;

                // The main file even when reading the config failed, since that is when
                // seeing it helps most.
                std::vector<std::filesystem::path> paths;
                if (loaded.config)
                    paths.push_back(std::filesystem::absolute(loaded.config->file).lexically_normal());
                if (recording)
                    for (const auto& path : recording->files)
                        if (!std::ranges::contains(paths, path))
                            paths.push_back(path);
                for (const auto& path : paths)
                    keybindings->sources.push_back(readSourceFile(path));
            }
            return [this, keybindings = std::move(keybindings)] { listeners.keybindings(keybindings); };
        });
    }

}
