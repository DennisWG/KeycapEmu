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
#include <realm.hpp>

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

    void client_connection::send_error(protocol::auth_result result)
    {
        protocol::server_logon_challange_error error;
        error.result = result;

        send(error.encode());
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
        constexpr auto header_size = sizeof(byte) + sizeof(byte) + sizeof(uint16);
        constexpr auto error_position = sizeof(byte) + sizeof(byte);

        if (stream.size() < header_size || stream.peek<uint16>(error_position) > (stream.size() - header_size))
            return client_connection::state_result::IncompleteData;

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

        connection.service_locator().send_registered(
            shared_net::account_service, request.encode(),
            [&, account_name = packet.account_name ](net::service_type sender, net::memory_stream data) {
                auto self = std::static_pointer_cast<client_connection>(connection.shared_from_this());
                on_account_reply(self, data, account_name);
                return true;
            });

        return client_connection::state_result::Ok;
    }

    void client_connection::just_connected::on_account_reply(std::weak_ptr<client_connection> connection,
                                                             keycap::root::network::memory_stream& stream,
                                                             std::string const& account_name)
    {
        if (connection.expired())
            return;

        auto conn = connection.lock();

        auto logger = keycap::root::utility::get_safe_logger("connections");
        auto reply = shared_net::reply_account_data::decode(stream);

        if (!reply.has_data)
        {
            conn->send_error(protocol::auth_result::InvalidInfo);
            logger->info("User {} tried to log in with incorrect login info!", account_name);
            return;
        }

        constexpr auto compliance = net::srp6::compliance::Wow;
        auto parameter = net::srp6::get_parameters(net::srp6::group_parameters::_256);

        Botan::BigInt verifier{reply.data->verifier};
        Botan::BigInt salt{reply.data->salt};

        challanged_data challanged_data;
        challanged_data.server = std::make_shared<net::srp6::server>(parameter, verifier, compliance);
        challanged_data.v = verifier;
        challanged_data.username = account_name;
        challanged_data.user_salt = salt;
        challanged_data.checksum_salt = Botan::AutoSeeded_RNG().random_vec(16);
        challanged_data.account_flags = static_cast<protocol::account_flag>(reply.data->flags);

        send_server_challange(conn, challanged_data, compliance, parameter, salt, reply.data->security_options);
        conn->state_ = challanged{challanged_data};
    }

    void client_connection::just_connected::send_server_challange(std::shared_ptr<client_connection> conn,
                                                                  challanged_data const& challanged_data,
                                                                  net::srp6::compliance compliance,
                                                                  net::srp6::group_parameter const& parameter,
                                                                  Botan::BigInt const& salt, uint8 security_options)
    {
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
        outPacket.security_flags = protocol::security_flag{security_options};
        std::copy(std::begin(challanged_data.checksum_salt), std::end(challanged_data.checksum_salt),
                  std::begin(outPacket.checksum_salt));

        conn->send(outPacket.encode());
    }

    void update_session_key(client_connection& connection, std::vector<uint8> session_key)
    {
        shared_net::update_session_key update;
        update.session_key = keycap::root::utility::to_hex_string(session_key.begin(), session_key.end());

        connection.service_locator().send_to(shared_net::account_service, update.encode());
    }

    client_connection::state_result client_connection::challanged::on_data(client_connection& connection,
                                                                           net::data_router const& router,
                                                                           net::memory_stream& stream)
    {
        if (stream.size() < protocol::client_logon_proof::expected_size)
            return client_connection::state_result::IncompleteData;

        if (stream.peek<protocol::command>() != protocol::command::Proof)
            return client_connection::state_result::Abort;

        auto packet{protocol::client_logon_proof::decode(stream)};
        stream.shrink();

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.to_string());

        auto[session_key, M1, M1_S] = generate_session_key_and_server_proof(packet);

        if (M1_S != M1)
        {
            connection.send_error(protocol::auth_result::InvalidInfo);
            logger->info("User {} tried to log in with incorrect login info!", data.username);
            return client_connection::state_result::Ok;
        }

        update_session_key(connection, session_key);

        send_proof_success(connection, session_key, M1_S);

        connection.state_ = authenticated{};
        return client_connection::state_result::Ok;
    }

    std::tuple<std::vector<uint8_t>, Botan::BigInt, Botan::BigInt>
    client_connection::challanged::generate_session_key_and_server_proof(protocol::client_logon_proof const& packet)
    {
        Botan::BigInt A{packet.A.data(), 32};
        Botan::BigInt M1{packet.M1.data(), 20};
        auto session_key = data.server->session_key(A);

        auto M1_S = net::srp6::generate_client_proof(data.server->prime(), data.server->generator(), data.user_salt,
                                                     data.username, A, data.server->public_ephemeral_value(),
                                                     session_key, data.server->compliance_mode());

        return std::make_tuple(std::move(session_key), std::move(M1), std::move(M1_S));
    }

    void client_connection::challanged::send_proof_success(client_connection& connection,
                                                           std::vector<uint8_t> session_key, Botan::BigInt M1_S)
    {
        protocol::server_logon_proof outPacket;
        outPacket.cmd = protocol::command::Proof;
        outPacket.error_code = protocol::error::Success;
        outPacket.M2 = net::srp6::to_array<20>(data.server->proof(M1_S, session_key), data.server->compliance_mode());
        outPacket.account_flags = data.account_flags;

        connection.send(outPacket.encode());
    }

    client_connection::state_result client_connection::authenticated::on_data(client_connection& connection,
                                                                              net::data_router const& router,
                                                                              net::memory_stream& stream)
    {
        if (stream.size() < protocol::client_realm_list::expected_size)
            return client_connection::state_result::IncompleteData;

        if (stream.peek<protocol::command>() != protocol::command::RealmList)
            return client_connection::state_result::Abort;

        auto packet{protocol::client_realm_list::decode(stream)};
        stream.shrink();

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
        data.category = protocol::realm_category::tournament;
        data.id = 1;

        auto& data2 = outPacket.data.emplace_back(protocol::realm_list_data{});
        data2.type = protocol::realm_type::PvE;
        data2.locked = 0;
        data2.realm_flags = protocol::realm_flag::New;
        data2.name = "KeycapEmu Testrealm 2";
        data2.ip = "127.0.0.1:8085";
        data2.population = 0.f;
        data2.num_characters = 0;
        data2.category = protocol::realm_category::test_server_2;
        data2.id = 2;

        connection.send(outPacket.encode());

        return client_connection::state_result::Ok;
    }
}
