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

#include <generated/shared_protocol.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;
namespace protocol = keycap::protocol;

namespace keycap::logonserver
{
    void update_session_key(client_connection& connection, std::string const& account_name,
                            Botan::BigInt const& session_key)
    {
        protocol::update_session_key update;
        update.account_name = account_name;
        auto key = Botan::BigInt::encode(session_key);
        update.session_key = keycap::root::utility::to_hex_string(key.begin(), key.end(), true);

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug("[client_connection] Updating session key of {} to {}", update.account_name, update.session_key);

        connection.service_locator().send_to(shared_net::account_service_type, update.encode());
    }

    shared::network::state_result client_connection::challanged::on_data(client_connection& connection,
                                                                         net::data_router const& router,
                                                                         net::memory_stream& stream)
    {
        if (stream.size() < protocol::client_logon_proof::expected_size)
            return shared::network::state_result::incomplete_data;

        if (stream.peek<protocol::command>() != protocol::command::proof)
            return shared::network::state_result::abort;

        auto packet{protocol::client_logon_proof::decode(stream)};

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.to_string());

        auto [session_key, M1, M1_S] = generate_session_key_and_server_proof(packet);

        if (M1_S != M1)
        {
            connection.send_error(protocol::grunt_result::unknown_account);
            logger->info("User {} tried to log in with incorrect login info!", data.username);
            return shared::network::state_result::ok;
        }

        update_session_key(connection, data.username, session_key);

        send_proof_success(connection, session_key, M1_S);

        connection.state_ = authenticated{};
        return shared::network::state_result::ok;
    }
}