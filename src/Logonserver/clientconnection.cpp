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

#include "ClientConnection.hpp"

#include <keycap/root/network/srp6/server.hpp>
#include <keycap/root/network/srp6/utility.hpp>
#include <keycap/root/utility/scope_exit.hpp>

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

namespace net = keycap::root::network;

namespace Keycap::Logonserver
{
    ClientConnection::ClientConnection(net::service_base& service)
      : BaseConnection{service}
    {
        router_.configure_inbound(this);
    }

    bool ClientConnection::on_data(net::service_base& service, std::vector<uint8_t> const& data)
    {
        inputStream_.put(gsl::make_span(data));
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.OnData(*this, service, inputStream_) != ClientConnection::StateResult::Abort;
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

    bool ClientConnection::on_link(net::service_base& service, net::link_status status)
    {
        if (status == net::link_status::Up)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("New connection");
            state_ = JustConnected{};
        }
        else
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Connection closed");
            state_ = Disconnected{};
        }

        return true;
    }

    ClientConnection::StateResult ClientConnection::Disconnected::OnData(ClientConnection& connection,
                                                                         net::service_base& service,
                                                                         net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("defuq???");
        return ClientConnection::StateResult::Abort;
    }

    ClientConnection::StateResult ClientConnection::JustConnected::OnData(ClientConnection& connection,
                                                                          net::service_base& service,
                                                                          net::memory_stream& stream)
    {
        if (stream.peek<Protocol::Command>() != Protocol::Command::Challange)
            return ClientConnection::StateResult::Abort;

        auto packet{Protocol::ClientLogonChallange::Decode(stream)};
        stream.shrink();

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.ToString());

        if (packet.command != Protocol::Command::Challange)
            return ClientConnection::StateResult::Abort;

        constexpr auto compliance = net::srp6::compliance::Wow;

        // todo: generate a propper salt. store it in a database and load it
        Botan::BigInt salt = Botan::BigInt::decode({Botan::AutoSeeded_RNG().random_vec(32)});
        auto parameter = net::srp6::get_parameters(net::srp6::group_parameters::_256);

        auto v = net::srp6::generate_verifier("user", "password", parameter, salt, compliance);

        ChallangedData challangedData;
        challangedData.server = std::make_shared<net::srp6::server>(parameter, v, compliance);
        challangedData.v = v;
        challangedData.username = packet.accountName;
        challangedData.userSalt = salt;
        challangedData.checksumSalt = Botan::AutoSeeded_RNG().random_vec(16);

        Protocol::ServerLogonChallange outPacket{};
        outPacket.command = Protocol::Command::Challange;
        outPacket.error = Protocol::Error::Success;
        outPacket.authResult = Protocol::AuthResult::Ok;
        outPacket.B = net::srp6::to_array<32>(challangedData.server->public_ephemeral_value(), compliance);
        outPacket.g_length = 1;
        outPacket.g = static_cast<uint8_t>(parameter.g);
        outPacket.N_length = 32;
        outPacket.N = net::srp6::to_array<32>(Botan::BigInt{parameter.N}, compliance);
        outPacket.s = net::srp6::to_array<32>(salt, compliance);
        outPacket.securityFlags = Protocol::SecurityFlags::None;
        std::copy(std::begin(challangedData.checksumSalt), std::end(challangedData.checksumSalt),
                  std::begin(outPacket.checksumSalt));

        net::memory_stream buffer;
        outPacket.Encode(buffer);

        connection.send(std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size()));

        connection.state_ = Challanged{challangedData};

        return ClientConnection::StateResult::Ok;
    }

    ClientConnection::StateResult ClientConnection::Challanged::OnData(ClientConnection& connection,
                                                                       net::service_base& service,
                                                                       net::memory_stream& stream)
    {
        if (stream.peek<Protocol::Command>() != Protocol::Command::Proof)
            return ClientConnection::StateResult::Abort;

        auto packet{Protocol::ClientLogonProof::Decode(stream)};
        stream.shrink();

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.ToString());

        Botan::BigInt A{packet.A.data(), 32};
        Botan::BigInt M1{packet.M1.data(), 20};
        auto sessionKey = data.server->session_key(A);

        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        for (auto&& v : sessionKey)
            ss << std::setw(2) << (int)v;

        // auto& db = GetLoginDatabase();

        // db.Query("UPDATE account SET sessionKey = '" + ss.str() + "' WHERE username = '" + data.username + "'",
        // [](auto a, auto b){});

        auto M1_S = net::srp6::generate_client_proof(data.server->prime(), data.server->generator(), data.userSalt,
                                                     data.username, A, data.server->public_ephemeral_value(),
                                                     sessionKey, data.server->compliance_mode());

        if (M1_S != M1)
        {
            logger->error("User {} tried to log in with incorrect login info!", data.username);
            return ClientConnection::StateResult::Abort;
        }

        Protocol::ServerLogonProof outPacket;
        outPacket.command = Protocol::Command::Proof;
        outPacket.error = Protocol::Error::Success;
        outPacket.M2 = net::srp6::to_array<20>(data.server->proof(M1_S, sessionKey), data.server->compliance_mode());
        outPacket.accountFlags = Protocol::AccountFlags::ProPass;

        net::memory_stream buffer;
        outPacket.Encode(buffer);

        connection.send(std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size()));

        connection.state_ = Authenticated{};
        return ClientConnection::StateResult::Ok;
    }

    ClientConnection::StateResult ClientConnection::Authenticated::OnData(ClientConnection& connection,
                                                                          net::service_base& service,
                                                                          net::memory_stream& stream)
    {
        if (stream.peek<Protocol::Command>() != Protocol::Command::RealmList)
            return ClientConnection::StateResult::Abort;

        auto packet{Protocol::ClientRealmList::Decode(stream)};
        stream.shrink();

        stream.put(stream);

        auto logger = keycap::root::utility::get_safe_logger("connections");
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

        net::memory_stream buffer;
        outPacket.Encode(buffer);

        auto begin = buffer.data();
        auto end = begin + buffer.size();
        std::stringstream ss;
        keycap::root::utility::dump_as_hex(begin, end, ss);
        logger->debug(ss.str());

        connection.send(std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size()));

        return ClientConnection::StateResult::Ok;
    }
}
