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
#include "../../realm_manager.hpp"

#include <generated/shared_protocol.hpp>

#include <keycap/root/network/srp6/server.hpp>
#include <keycap/root/network/srp6/utility.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;

namespace keycap::logonserver
{
    std::tuple<Botan::BigInt, Botan::BigInt, Botan::BigInt>
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

    void client_connection::challanged::send_proof_success(client_connection& connection, Botan::BigInt session_key,
                                                           Botan::BigInt M1_S)
    {
        protocol::server_logon_proof outPacket;
        outPacket.M2 = net::srp6::to_array<20>(data.server->proof(M1_S, session_key), data.server->compliance_mode());
        outPacket.account_flags = data.account_flags;
        outPacket.num_account_messages = 1;

        connection.send(outPacket.encode());
    }

    shared::network::state_result client_connection::authenticated::on_data(client_connection& connection,
                                                                            net::data_router const& router,
                                                                            net::memory_stream& stream)
    {
        if (stream.size() < protocol::client_realm_list::expected_size)
            return shared::network::state_result::incomplete_data;

        if (stream.peek<protocol::command>() != protocol::command::realm_list)
            return shared::network::state_result::abort;

        auto packet{protocol::client_realm_list::decode(stream)};

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug("[client_connection] {}", packet.to_string());

        protocol::server_realm_list outPacket;

        /*
        connection.io_service_.post([conn=connection.weak_from_this()](){
            if(conn.expired())
                return;
        });
        */

        connection.realm_manager_.iterate([&outPacket](keycap::protocol::realm_info const& realm_data) {
            auto& data = outPacket.data.emplace_back(protocol::realm_list_data{});
            data.type = realm_data.type;
            data.locked = realm_data.locked;
            data.realm_flags = realm_data.realm_flags;
            data.name = realm_data.name;
            data.ip = realm_data.ip;
            data.population = realm_data.population;
            data.num_characters = 0;
            data.category = realm_data.category;
            data.id = realm_data.id;
        });

        outPacket.unk = 0xC01A;

        connection.send(outPacket.encode());

        return shared::network::state_result::ok;
    }

    void client_connection::authenticated::send_realm_list()
    {
    }
}