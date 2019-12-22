/*
    Copyright 2018-2019 KeycapEmu

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "../client_connection.hpp"

#include <keycap/root/utility/md5.hpp>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

namespace net = keycap::root::network;

namespace keycap::logonserver
{
    client_connection::transferring::transferring(std::weak_ptr<client_connection> connection)
      : state{connection}
    {
        protocol::xfer_initiate packet;

        // TODO: Implement proper file handling. See https://github.com/DennisWG/KeycapEmu/issues/21
        constexpr auto path = "Survey.mpq";
        auto size = std::filesystem::file_size(path);

        std::ifstream file{path, std::ios::binary};

        data.resize(static_cast<size_t>(size), 0);
        file.read(reinterpret_cast<char*>(data.data()), size);

        if (!file.good())
            throw "Something went wrong!";

        packet.action = "Survey";
        packet.filesize = static_cast<uint64>(size);
        packet.md5_checksum = root::utility::md5(data);

        connection.lock()->send(packet.encode());
    }

    shared::network::state_result client_connection::transferring::on_data(net::data_router const& router,
                                                                           net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");

        auto command = stream.peek<protocol::command>();
        auto conn = connection.lock();

        switch (command)
        {
            // Will be send when "C:\Users\Public\Documents\Blizzard Entertainment\World of Warcraft" doesn't exist on
            // Windows Vista or later. For XP "C:\Documents and Settings\All Users\Documents" must exist.
            // Or, when Survey.MPQ already exists locally
            case protocol::command::xfer_cancel:
                stream.clear();
                conn->state_ = authenticated{connection};
                return shared::network::state_result::ok;

            case protocol::command::xfer_accept:
                stream.clear();
                send({data.begin(), data.end()});
                break;

            case protocol::command::xfer_resume:
                // TODO: https://github.com/DennisWG/KeycapEmu/issues/6
                stream.clear();
                break;

            default:
                logger->debug("Received unexpected command {}({})", command.to_string(), command);
                break;
        }

        return shared::network::state_result::ok;
    }

    void client_connection::transferring::send(keycap::root::network::memory_stream const& stream)
    {
        auto conn = connection.lock();

        if (!stream.has_data_remaining())
        {
            // conn->state_ = authenticated{connection};
            return;
        }

        auto callback = [this, stream = stream, connection = conn]() mutable {
            protocol::xfer_data packet;

            size_t constexpr max_packet_size = 65535;
            auto size = std::min(stream.size(), max_packet_size);

            packet.chunk.resize(size);
            stream.write_to(packet.chunk.begin(), size);

            connection->send(packet.encode());
            send(stream);
        };

        conn->io_service_.post(callback);
    }
}