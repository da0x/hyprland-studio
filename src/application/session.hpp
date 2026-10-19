// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/worker.hpp"
#include "ipc/bindings.hpp"
#include "ipc/events.hpp"
#include "ipc/instance.hpp"

#include <chrono>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace application {

    // What the session needs from the UI's event loop. The UI provides it, so this layer
    // knows nothing about the toolkit.
    struct EventLoop {
        std::function<void(int descriptor, std::function<void()> readable)>        watch;
        std::function<void(int descriptor)>                                         unwatch;
        std::function<void(std::chrono::milliseconds delay, std::function<void()>)> after;
    };

    // Whether what is on screen is still Hyprland's current state.
    enum class Connection {
        connecting,
        live,
        lost,
    };

    struct Listeners {
        std::function<void(Connection, const std::string& reason)>                          connection;
        std::function<void(const std::expected<std::vector<ipc::Binding>, std::string>&)> bindings;
    };

    // Keeps the studio's picture of the running Hyprland current. It listens on the event
    // socket and asks again only when an event says something changed. It never polls.
    class Session {
      public:
        Session(EventLoop loop, Listeners listeners);
        ~Session();
        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;

        void start();

      private:
        void                               connect();
        void                               lose(const std::string& reason);
        void                               readEvents();
        void                               refreshBindings();

        EventLoop                          loop;
        Listeners                          listeners;
        Worker                             worker;
        std::optional<ipc::Instance>       instance;
        std::optional<ipc::EventConnection> events;
    };

}
