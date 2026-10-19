// SPDX-License-Identifier: GPL-3.0-or-later
#include "ipc/request.hpp"

#include "ipc/socket.hpp"

#include <array>
#include <format>
#include <string_view>

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace ipc {

    namespace {

        std::string_view command(Query query) {
            switch (query) {
                case Query::bindings: return "j/binds";
            }
            return {};
        }

    }

    std::expected<std::string, std::string> request(const Instance& instance, Query query) {
        auto socket = connectTo(instance.requestSocket);
        if (!socket)
            return std::unexpected{socket.error()};

        // Hyprland gives up on a request after 5 seconds, so waiting longer gains nothing.
        const timeval timeout{.tv_sec = 5, .tv_usec = 0};
        ::setsockopt(socket->get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        ::setsockopt(socket->get(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        const auto text = command(query);
        if (::write(socket->get(), text.data(), text.size()) != static_cast<ssize_t>(text.size()))
            return std::unexpected{std::format("cannot send \"{}\": {}", text, lastError())};

        std::string            reply;
        std::array<char, 8192> buffer{};
        while (true) {
            const auto count = ::read(socket->get(), buffer.data(), buffer.size());
            if (count == 0)
                return reply;
            if (count < 0)
                return std::unexpected{std::format("no reply to \"{}\": {}", text, lastError())};
            reply.append(buffer.data(), static_cast<std::size_t>(count));
        }
    }

}
