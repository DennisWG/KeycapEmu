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
#include <Keycap/Root/Network/Srp6/Utility.hpp>
#include <Keycap/Root/Utility/ScopeExit.hpp>

#include <Database/Database.hpp>

#include <spdlog/fmt/bundled/ostream.h>
#include <spdlog/spdlog.h>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/sha160.h>

extern Keycap::Shared::Database::Database& GetLoginDatabase();

std::ostream& operator<<(std::ostream& os, std::vector<uint8_t> const& vec)
{
    os << std::hex;

    SCOPE_EXIT(sc, [&os]() { os << std::dec; });

    for (auto&& element : vec)
        os << static_cast<int>(element);

    return os;
}

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
        if (stream.Peek<Protocol::Command>() != Protocol::Command::Challange)
            return Connection::StateResult::Abort;

        auto packet{Protocol::ClientLogonChallange::Decode(stream)};
        stream.Shrink();

        auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
        logger->debug(packet.ToString());

        if (packet.command != Protocol::Command::Challange)
            return Connection::StateResult::Abort;

        constexpr auto compliance = net::Srp6::Compliance::Wow;

        // todo: generate a propper salt. store it in a database and load it
        Botan::BigInt salt = Botan::BigInt::decode({Botan::AutoSeeded_RNG().random_vec(32)});
        auto parameter = net::Srp6::GetParameters(net::Srp6::GroupParameters::_256);

        auto v = net::Srp6::GenerateVerifier("user", "password", parameter, salt, compliance);

        ChallangedData challangedData;
        challangedData.server = std::make_shared<net::Srp6::Server>(parameter, v, compliance);
        challangedData.v = v;
        challangedData.username = packet.accountName;
        challangedData.userSalt = salt;
        challangedData.checksumSalt = Botan::AutoSeeded_RNG().random_vec(16);

        Protocol::ServerLogonChallange outPacket{};
        outPacket.command = Protocol::Command::Challange;
        outPacket.error = Protocol::Error::Success;
        outPacket.authResult = Protocol::AuthResult::Ok;
        outPacket.B = net::Srp6::ToArray<32>(challangedData.server->PublicEphemeralValue(), compliance);
        outPacket.g_length = 1;
        outPacket.g = static_cast<uint8_t>(parameter.g);
        outPacket.N_length = 32;
        outPacket.N = net::Srp6::ToArray<32>(Botan::BigInt{parameter.N}, compliance);
        outPacket.s = net::Srp6::ToArray<32>(salt, compliance);
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

        Botan::BigInt A{packet.A.data(), 32};
        Botan::BigInt M1{packet.M1.data(), 20};
        auto sessionKey = data.server->SessionKey(A);

        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        for (auto&& v : sessionKey)
            ss << std::setw(2) << (int)v;

        auto& db = GetLoginDatabase();

        //db.Query("UPDATE account SET sessionKey = '" + ss.str() + "' WHERE username = '" + data.username + "'", [](auto a, auto b){});

        auto M1_S = net::Srp6::GenerateClientProof(data.server->Prime(), data.server->Generator(), data.userSalt,
                                                   data.username, A, data.server->PublicEphemeralValue(), sessionKey,
                                                   data.server->ComplianceMode());

        if (M1_S != M1)
        {
            logger->error("User {} tried to log in with incorrect login info!", data.username);
            return Connection::StateResult::Abort;
        }


        Protocol::ServerLogonProof outPacket;
        outPacket.command = Protocol::Command::Proof;
        outPacket.error = Protocol::Error::Success;
        outPacket.M2 = net::Srp6::ToArray<20>(data.server->Proof(M1_S, sessionKey), data.server->ComplianceMode());
        outPacket.accountFlags = Protocol::AccountFlags::ProPass;

        net::MemoryStream buffer;
        outPacket.Encode(buffer);

        connection.Send(std::vector<uint8_t>(buffer.Data(), buffer.Data() + buffer.Size()));

        connection.state_ = Authenticated{};
        return Connection::StateResult::Ok;
    }

    Connection::StateResult Connection::Authenticated::OnData(Connection& connection, net::ServiceBase& service,
                                                              net::MemoryStream& stream)
    {
        if (stream.Peek<Protocol::Command>() != Protocol::Command::RealmList)
            return Connection::StateResult::Abort;

        auto packet{Protocol::ClientRealmList::Decode(stream)};
        stream.Shrink();

        stream.Put(stream);

        auto logger = Keycap::Root::Utility::GetSafeLogger("connections");
        logger->debug(packet.ToString());

        Protocol::ServerRealmList outPacket;
        auto& data = outPacket.data.emplace_back(Protocol::RealmListData{});
        data.type = Protocol::RealmType::PvE;
        data.locked = 0;
        data.flags = Protocol::RealmFlags::Recommended;
        data.name = "KeycapEmu Testrealm";
        data.ip = "127.0.0.1:8085";
        data.population = 0.f;
        data.numCharacters = 0;
        data.category = 5;
        data.id = 1;

        auto& data2 = outPacket.data.emplace_back(Protocol::RealmListData{});
        data2.type = Protocol::RealmType::PvE;
        data2.locked = 0;
        data2.flags = Protocol::RealmFlags::New;
        data2.name = "KeycapEmu Testrealm 2";
        data2.ip = "127.0.0.1:8086";
        data2.population = 0.f;
        data2.numCharacters = 0;
        data2.category = 1;
        data2.id = 2;

        outPacket.unk = 0xACAB;

        net::MemoryStream buffer;
        outPacket.Encode(buffer);

        auto begin = buffer.Data();
        auto end = begin + buffer.Size();
        std::stringstream ss;
        Keycap::Root::Utility::DumpAsHex(begin, end, ss);
        logger->debug(ss.str());

        connection.Send(std::vector<uint8_t>(buffer.Data(), buffer.Data() + buffer.Size()));

        return Connection::StateResult::Ok;
    }
}
