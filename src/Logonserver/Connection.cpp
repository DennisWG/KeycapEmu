/*
    Copyright 2017 KeycapEmu

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

#include "Connection.hpp"

namespace net = Keycap::Root::Network;

namespace Keycap::Logonserver
{
    Connection::Connection(net::ServiceBase& service)
      : BaseConnection{service}
    {
        router_.ConfigureInbound(this);
    }

    bool Connection::OnData(net::ServiceBase& service, std::vector<uint8_t> const& data)
    {
        inputStream_.Put(gsl::make_span(data));
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = Keycap::Root::Utility::GetSafeLogger("Connections");

            std::cout << state.name << '\n';
            return state.OnData(*this, service, inputStream_) != Connection::StateResult::Abort;
        }, state_);
        // clang-format on
    }

    bool Connection::OnLink(net::ServiceBase& service, net::LinkStatus status)
    {
        if (status == net::LinkStatus::Up)
        {
            std::cout << "New connection\n";
            state_ = JustConnected{};
        }
        else
        {
            std::cout << "Connection closed\n";
            state_ = Disconnected{};
        }

        return true;
    }

    Connection::StateResult Connection::Disconnected::OnData(Connection& connection, net::ServiceBase& service,
                                                             net::MemoryStream& stream)
    {
        return Connection::StateResult::Abort;
    }

    Connection::StateResult Connection::JustConnected::OnData(Connection& connection, net::ServiceBase& service,
                                                              net::MemoryStream& stream)
    {
        Protocol::ClientLogonChallange packet;
        packet.decode(stream);
        stream.Shrink();

        if (packet.command != Protocol::Command::Challange)
            return Connection::StateResult::Abort;

        Protocol::ServerLogonChallange outPacket{};
        outPacket.command = Protocol::Command::Challange;
        outPacket.error = Protocol::Error::None;
        outPacket.authResult = Protocol::AuthResult::Ok;
        outPacket.B = {}; // todo
        outPacket.g_length = 1;
        outPacket.g = 0; // todo
        outPacket.N_length = 32;
        outPacket.N = {}; // todo
        outPacket.s = {}; // todo
        outPacket.checksumSalt = {}; // todo
        outPacket.securityFlags = Protocol::SecurityFlags::None;

        net::MemoryStream buffer;
        outPacket.encode(buffer);

        connection.Send(std::vector<uint8_t>(buffer.Data(), buffer.Data() + buffer.Size()));

        return Connection::StateResult::Ok;
    }

    Connection::StateResult Connection::Challanged::OnData(Connection& connection, net::ServiceBase& service,
                                                           net::MemoryStream& stream)
    {
        return Connection::StateResult::Abort;
    }

    Connection::StateResult Connection::Authenticated::OnData(Connection& connection, net::ServiceBase& service,
                                                              net::MemoryStream& stream)
    {
        return Connection::StateResult::Abort;
    }
}