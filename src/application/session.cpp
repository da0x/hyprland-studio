// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/session.hpp"

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
        refreshBindings();
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
        if (std::ranges::any_of(*drained, [](const ipc::Event& event) { return event.name == "configreloaded"; }))
            refreshBindings();
    }

    void Session::refreshBindings() {
        worker.post([this, instance = *instance]() -> Worker::Continuation {
            const auto reply = ipc::request(instance, ipc::Query::bindings);
            auto       parsed = reply ? ipc::parseBindings(*reply) : std::expected<std::vector<ipc::Binding>, std::string>{std::unexpect, reply.error()};
            return [this, parsed = std::move(parsed)] { listeners.bindings(parsed); };
        });
    }

}
