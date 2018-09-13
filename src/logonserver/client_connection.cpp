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

#include "client_connection.hpp"

#include <protocol.hpp>

#include <keycap/root/network/srp6/server.hpp>
#include <keycap/root/network/srp6/utility.hpp>
#include <keycap/root/utility/scope_exit.hpp>

#include <network/services.hpp>

#include <spdlog/fmt/bundled/ostream.h>
#include <spdlog/spdlog.h>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/sha160.h>

std::ostream& operator<<(std::ostream& os, std::vector<uint8_t> const& vec)
{
    os << std::hex;

    SCOPE_EXIT(sc, [&os]() { os << std::dec; });

    for (auto&& element : vec)
        os << static_cast<int>(element);

    return os;
}

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;

namespace keycap::logonserver
{
    client_connection::client_connection(net::service_base& service, keycap::root::network::service_locator& locator)
      : base_connection{service}
      , locator_{locator}
    {
        router_.configure_inbound(this);
    }

    bool client_connection::on_data(net::data_router const& router, std::vector<uint8_t> const& data)
    {
        input_stream_.put(gsl::make_span(data));
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.on_data(*this, router, input_stream_) != client_connection::state_result::Abort;
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

    bool client_connection::on_link(net::data_router const& router, net::link_status status)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");

        if (status == net::link_status::Up)
        {
            logger->debug("New connection");
            state_ = just_connected{};
        }
        else
        {
            logger->debug("Connection closed");
            state_ = disconnected{};
        }

        return true;
    }

    client_connection::state_result client_connection::disconnected::on_data(client_connection& connection,
                                                                             net::data_router const& router,
                                                                             net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("defuq???");
        return client_connection::state_result::Abort;
    }

    client_connection::state_result client_connection::just_connected::on_data(client_connection& connection,
                                                                               net::data_router const& router,
                                                                               net::memory_stream& stream)
    {
        if (stream.peek<protocol::command>() != protocol::command::Challange)
            return client_connection::state_result::Abort;

        auto packet{protocol::client_logon_challange::decode(stream)};
        stream.shrink();

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.to_string());

        if (packet.cmd != protocol::command::Challange)
            return client_connection::state_result::Abort;

        shared_net::request_account_data request;
        request.account_name = packet.account_name;

        net::memory_stream stream2;
        request.encode(stream2);

        connection.service_locator().send_registered(
            shared_net::account_service, stream2,
            [&, account_name = packet.account_name ](net::service_type sender, net::memory_stream data) {
                on_account_reply(connection, data, account_name);
                return true;
            });

        return client_connection::state_result::Ok;
    }

    void client_connection::just_connected::on_account_reply(client_connection& connection,
                                                             keycap::root::network::memory_stream& stream,
                                                             std::string const& account_name)
    {
        auto reply = shared_net::reply_account_data::decode(stream);

        if (reply.verifier.empty() || reply.salt.empty())
        {
            connection.socket().close();
            return;
        }

        constexpr auto compliance = net::srp6::compliance::Wow;
        auto parameter = net::srp6::get_parameters(net::srp6::group_parameters::_256);

        Botan::BigInt verifier{reply.verifier};
        Botan::BigInt salt{reply.salt};

        challanged_data challanged_data;
        challanged_data.server = std::make_shared<net::srp6::server>(parameter, verifier, compliance);
        challanged_data.v = verifier;
        challanged_data.username = account_name;
        challanged_data.user_salt = salt;
        challanged_data.checksum_salt = Botan::AutoSeeded_RNG().random_vec(16);

        protocol::server_logon_challange outPacket{};
        outPacket.cmd = protocol::command::Challange;
        outPacket.error_code = protocol::error::Success;
        outPacket.result = protocol::auth_result::Ok;
        outPacket.B = net::srp6::to_array<32>(challanged_data.server->public_ephemeral_value(), compliance);
        outPacket.g_length = 1;
        outPacket.g = static_cast<uint8_t>(parameter.g);
        outPacket.N_length = 32;
        outPacket.N = net::srp6::to_array<32>(Botan::BigInt{parameter.N}, compliance);
        outPacket.s = net::srp6::to_array<32>(salt, compliance);
        outPacket.security_flags = protocol::security_flag::None;
        std::copy(std::begin(challanged_data.checksum_salt), std::end(challanged_data.checksum_salt),
                  std::begin(outPacket.checksum_salt));

        net::memory_stream buffer;
        outPacket.encode(buffer);

        connection.send(std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size()));

        connection.state_ = challanged{challanged_data};
    }

    client_connection::state_result client_connection::challanged::on_data(client_connection& connection,
                                                                           net::data_router const& router,
                                                                           net::memory_stream& stream)
    {
        if (stream.peek<protocol::command>() != protocol::command::Proof)
            return client_connection::state_result::Abort;

        auto packet{protocol::client_logon_proof::decode(stream)};
        stream.shrink();

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.to_string());

        Botan::BigInt A{packet.A.data(), 32};
        Botan::BigInt M1{packet.M1.data(), 20};
        auto sessionKey = data.server->session_key(A);

        auto M1_S = net::srp6::generate_client_proof(data.server->prime(), data.server->generator(), data.user_salt,
                                                     data.username, A, data.server->public_ephemeral_value(),
                                                     sessionKey, data.server->compliance_mode());

        if (M1_S != M1)
        {
            logger->error("User {} tried to log in with incorrect login info!", data.username);
            return client_connection::state_result::Abort;
        }

        protocol::server_logon_proof outPacket;
        outPacket.cmd = protocol::command::Proof;
        outPacket.error_code = protocol::error::Success;
        outPacket.M2 = net::srp6::to_array<20>(data.server->proof(M1_S, sessionKey), data.server->compliance_mode());
        outPacket.account_flags = protocol::account_flag::ProPass;

        net::memory_stream buffer;
        outPacket.encode(buffer);

        connection.send(std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size()));

        connection.state_ = authenticated{};
        return client_connection::state_result::Ok;
    }

    client_connection::state_result client_connection::authenticated::on_data(client_connection& connection,
                                                                              net::data_router const& router,
                                                                              net::memory_stream& stream)
    {
        if (stream.peek<protocol::command>() != protocol::command::RealmList)
            return client_connection::state_result::Abort;

        auto packet{protocol::client_realm_list::decode(stream)};
        stream.shrink();

        stream.put(stream);

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.to_string());

        protocol::ServerRealmList outPacket;
        auto& data = outPacket.data.emplace_back(protocol::realm_list_data{});
        data.type = protocol::realm_type::PvE;
        data.locked = 0;
        data.realm_flags = protocol::realm_flag::Recommended;
        data.name = "KeycapEmu Testrealm";
        data.ip = "127.0.0.1:8085";
        data.population = 0.f;
        data.num_characters = 0;
        data.category = 5;
        data.id = 1;

        auto& data2 = outPacket.data.emplace_back(protocol::realm_list_data{});
        data2.type = protocol::realm_type::PvE;
        data2.locked = 0;
        data2.realm_flags = protocol::realm_flag::New;
        data2.name = "KeycapEmu Testrealm 2";
        data2.ip = "127.0.0.1:8086";
        data2.population = 0.f;
        data2.num_characters = 0;
        data2.category = 1;
        data2.id = 2;

        outPacket.unk = 0xACAB;

        net::memory_stream buffer;
        outPacket.encode(buffer);

        auto begin = buffer.data();
        auto end = begin + buffer.size();
        std::stringstream ss;
        keycap::root::utility::dump_as_hex(begin, end, ss);
        logger->debug(ss.str());

        connection.send(std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size()));

        return client_connection::state_result::Ok;
    }
}
