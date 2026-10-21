// SPDX-License-Identifier: GPL-3.0-or-later
#include "ipc/request.hpp"

#include "ipc/socket.hpp"

#include <algorithm>
#include <array>
#include <cctype>
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
                case Query::descriptions: return "j/descriptions";
                case Query::status: return "j/status";
            }
            return {};
        }

        // Sends `text` and waits for the whole reply. `label` names it in errors.
        std::expected<std::string, std::string> send(const Instance& instance, std::string_view text, std::string_view label) {
            auto socket = connectTo(instance.requestSocket);
            if (!socket)
                return std::unexpected{socket.error()};

            // Hyprland gives up on a request after 5 seconds, so waiting longer gains nothing.
            const timeval timeout{.tv_sec = 5, .tv_usec = 0};
            ::setsockopt(socket->get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            ::setsockopt(socket->get(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

            if (::write(socket->get(), text.data(), text.size()) != static_cast<ssize_t>(text.size()))
                return std::unexpected{std::format("cannot send \"{}\": {}", label, lastError())};

            std::string            reply;
            std::array<char, 8192> buffer{};
            while (true) {
                const auto count = ::read(socket->get(), buffer.data(), buffer.size());
                if (count == 0)
                    return reply;
                if (count < 0)
                    return std::unexpected{std::format("no reply to \"{}\": {}", label, lastError())};
                reply.append(buffer.data(), static_cast<std::size_t>(count));
            }
        }

    }

    std::expected<std::string, std::string> request(const Instance& instance, Query query) {
        return send(instance, command(query), command(query));
    }

    std::expected<std::string, std::string> requestOptions(const Instance& instance, const std::vector<std::string>& names) {
        std::string batch = "[[BATCH]]";
        for (const auto& name : names) {
            if (name.empty() || !std::ranges::all_of(name, [](char c) { return std::islower(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)) || std::string_view{"_.:-"}.contains(c); }))
                return std::unexpected{std::format("\"{}\" is not an option's name", name)};
            batch += std::format("{}j/getoption {}", batch.size() > 9 ? ";" : "", name);
        }
        return send(instance, batch, "j/getoption");
    }

}
