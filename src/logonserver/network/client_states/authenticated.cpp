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

#include "../../realm_manager.hpp"
#include "../client_connection.hpp"

#include <generated/shared_protocol.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;

namespace keycap::logonserver
{
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