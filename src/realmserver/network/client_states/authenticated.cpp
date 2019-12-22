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

#include <generated/authentication.hpp>
#include <generated/realm_protocol.hpp>

#include "../../handlers/character_handler.hpp"
#include "../client_connection.hpp"
#include "../handler.hpp"
#include "../player_session.hpp"

#include <keycap/root/network/srp6/utility.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace srp6 = keycap::root::network::srp6;

namespace keycap::realmserver
{
    client_connection::authenticated::authenticated(std::shared_ptr<client_connection> connection,
                                                    std::string const& account_name,
                                                    protocol::client_addon_info const& client_addons,
                                                    Botan::BigInt const& session_key)
    {
        auto logger = root::utility::get_safe_logger("connections");
        logger->trace("[client_connection::authenticated] authenticated::authenticated");

        connection->scrambler_.initialize(srp6::encode_flip(session_key));

        connection->player_session_
            = std::make_unique<player_session>(*connection, account_name, connection->scrambler_);

        connection->player_session_->send_addon_info(client_addons);

        connection->login_queue_.enqueue(connection->weak_from_this());
    }

    client_connection::state_result client_connection::authenticated::on_data(client_connection& connection,
                                                                              root::network::data_router const& router,
                                                                              root::network::memory_stream& stream)
    {
        auto [result, size, opcode] = connection.validate_packet_header(stream, &connection.scrambler_);
        if (result != shared::network::state_result::ok)
            return std::make_tuple(result, size, opcode);

        auto& handlers = realmserver::get_handlers();
        auto cmd = static_cast<uint32>(opcode.get());

        if (opcode == protocol::client_command::ping)
        {
            auto packet = protocol::ping::decode(stream);
            protocol::pong answer;
            answer.counter = packet.counter;

            connection.player_session_->send(answer.encode());
            return std::make_tuple(result, size, opcode);
        }

        character_handler ch{*connection.player_session_, connection.locator_};

        try
        {
            if (auto itr = handlers.find(cmd); itr != handlers.end())
                itr->second(*connection.player_session_, stream);
            else
            {
                auto logger = root::utility::get_safe_logger("connections");
                logger->error("[client_connection::on_data] Received unhandled packet (ID: {}, Name: {})!", cmd,
                              opcode.to_string());
            }
        }
        catch (std::exception const& ex)
        {
            auto logger = root::utility::get_safe_logger("connections");
            logger->trace("[client_connection::on_data] Exception thrown!", ex.what());

            return client_connection::state_result{shared::network::state_result::abort, size, opcode};
        }
        return std::make_tuple(result, size, opcode);
    }
}