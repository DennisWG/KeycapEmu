/*
    Copyright 2018 KeycapEmu

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

#include <Keycap/Root/Network/Srp6/Server.hpp>

#include <spdlog/spdlog.h>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/sha160.h>

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
            auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.OnData(*this, service, inputStream_) != Connection::StateResult::Abort;
            }
            catch (std::exception const& e)
            {
                logger->error(e.what());
                return false;
            }
            catch (...)
            {
                return false;
            }

            return false;
        }, state_);
        // clang-format on
    }

    bool Connection::OnLink(net::ServiceBase& service, net::LinkStatus status)
    {
        if (status == net::LinkStatus::Up)
        {
            auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
            logger->debug("New connection");
            state_ = JustConnected{};
        }
        else
        {
            auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
            logger->debug("Connection closed");
            state_ = Disconnected{};
        }

        return true;
    }

    Connection::StateResult Connection::Disconnected::OnData(Connection& connection, net::ServiceBase& service,
                                                             net::MemoryStream& stream)
    {
        auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
        logger->error("defuq???");
        return Connection::StateResult::Abort;
    }

    Connection::StateResult Connection::JustConnected::OnData(Connection& connection, net::ServiceBase& service,
                                                              net::MemoryStream& stream)
    {
        if(stream.Peek<Protocol::Command>() != Protocol::Command::Challange)
            return Connection::StateResult::Abort;

        auto packet{ Protocol::ClientLogonChallange::Decode(stream) };
        stream.Shrink();

        auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
        logger->debug(packet.ToString());


        if (packet.command != Protocol::Command::Challange)
            return Connection::StateResult::Abort;

        constexpr auto compliance = net::Srp6::Compliance::Wow;

        Botan::SHA_1 sha;
        sha.update("test");
        sha.update(":");
        sha.update("PASSWORD");
        auto xVec = sha.final();

        // todo: generate a propper salt. store it in a database and load it
        Botan::BigInt salt;

        auto encode = [](Botan::BigInt const& bn, net::Srp6::Compliance compliance) {
            if (compliance == net::Srp6::Compliance::RFC5054)
                return Botan::BigInt::encode(bn);
            else if (compliance == net::Srp6::Compliance::Wow)
            {
                std::vector<uint8_t> buffer{Botan::BigInt::encode(bn)};
                std::reverse(std::begin(buffer), std::end(buffer));
                return buffer;
            }

            throw std::exception("Unknown compliance mode!");
        };

        auto decode = [](Botan::secure_vector<uint8_t> vec, net::Srp6::Compliance compliance) {
            if (compliance == net::Srp6::Compliance::RFC5054)
                return Botan::BigInt::decode(vec);
            else if (compliance == net::Srp6::Compliance::Wow)
            {
                std::reverse(std::begin(vec), std::end(vec));
                return Botan::BigInt::decode(vec);
            }

            throw std::exception("Unknown compliance mode!");
        };

        auto toArray32 = [](Botan::BigInt value) {
            std::array<uint8_t, 32> array;
            auto tmp = Botan::BigInt::encode_1363(value, 32);
            std::reverse_copy(std::begin(tmp), std::end(tmp), std::begin(array));
            return array;
        };

        sha.update(encode(salt, compliance));

        xVec = sha.process(xVec);
        auto x = decode(xVec, compliance);

        auto parameter = net::Srp6::GetParameters(net::Srp6::GroupParameters::_256);
        Botan::BigInt N{parameter.value};
        Botan::BigInt g{parameter.generator};
        Botan::BigInt v = Botan::power_mod(g, x, N);

        net::Srp6::Server srp{parameter, v, compliance};

        ChallangedData challangedData;
        challangedData.groupParameters = parameter;
        challangedData.v = v;
        challangedData.compliance = compliance;
        challangedData.username = packet.accountName;
        challangedData.userSalt = salt;
        challangedData.checksumSalt = Botan::AutoSeeded_RNG().random_vec(16);

        Protocol::ServerLogonChallange outPacket{};
        outPacket.command = Protocol::Command::Challange;
        outPacket.error = Protocol::Error::Success;
        outPacket.authResult = Protocol::AuthResult::Ok;
        outPacket.B = toArray32(srp.PublicEphemeralValue());
        outPacket.g_length = 1;
        outPacket.g = static_cast<uint8_t>(parameter.generator);
        outPacket.N_length = 32;
        outPacket.N = toArray32(N);
        outPacket.s = toArray32(salt);
        outPacket.securityFlags = Protocol::SecurityFlags::None;
        std::copy(std::begin(challangedData.checksumSalt), std::end(challangedData.checksumSalt),
                  std::begin(outPacket.checksumSalt));

        net::MemoryStream buffer;
        outPacket.Encode(buffer);

        connection.Send(std::vector<uint8_t>(buffer.Data(), buffer.Data() + buffer.Size()));

        connection.state_ = Challanged{challangedData};

        return Connection::StateResult::Ok;
    }

    Connection::StateResult Connection::Challanged::OnData(Connection& connection, net::ServiceBase& service,
                                                           net::MemoryStream& stream)
    {
        if (stream.Peek<Protocol::Command>() != Protocol::Command::Proof)
            return Connection::StateResult::Abort;

        auto packet{Protocol::ClientLogonProof::Decode(stream)};
        stream.Shrink();

        auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
        logger->debug(packet.ToString());

        Protocol::ServerLogonProof outPacket;
        outPacket.command = Protocol::Command::Proof;
        outPacket.error = Protocol::Error::Success;
        //outPacket.M2 =
        outPacket.accountFlags = Protocol::AccountFlags::None;
        outPacket.surveyId = 0;
        outPacket.messageFlags = 0;

        net::MemoryStream buffer;
        outPacket.Encode(buffer);

        connection.Send(std::vector<uint8_t>(buffer.Data(), buffer.Data() + buffer.Size()));

        return Connection::StateResult::Abort;
    }

    Connection::StateResult Connection::Authenticated::OnData(Connection& connection, net::ServiceBase& service,
                                                              net::MemoryStream& stream)
    {
        return Connection::StateResult::Abort;
    }
}