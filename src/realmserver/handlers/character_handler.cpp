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

#include "character_handler.hpp"

#include "../network/player_session.hpp"
#include "../network/handler.hpp"

#include <generated/shared_protocol.hpp>

#include <database/daos/character.hpp>
#include <network/services.hpp>

#include <keycap/root/network/service_locator.hpp>

#include <spdlog/spdlog.h>

namespace db = keycap::shared::database;
namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;

extern boost::asio::io_service& get_net_service();
extern uint8 get_realm_id();

namespace keycap::realmserver
{
    character_handler::character_handler(player_session& session, keycap::root::network::service_locator& locator)
      : session_{session}
      , locator_{locator}
    {
        auto& handlers = get_handlers();

        keycap_add_handler(keycap::protocol::client_command::char_enum, this, &character_handler::handle_char_enum);
        keycap_add_handler(keycap::protocol::client_command::realm_split, this, &character_handler::handle_realm_split);
    }

    bool character_handler::handle_char_enum(keycap::protocol::client_char_enum packet)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->trace("[character_handler] handle_char_enum");

        keycap::protocol::request_characters request;
        request.realm_id = get_realm_id();
        request.account_id = session_.account_id();

        auto on_reply = [session = &session_](net::service_type sender, net::memory_stream data) {
            if (data.peek<keycap::protocol::shared_command>() != keycap::protocol::shared_command::reply_characters)
                return false;

            auto reply = keycap::protocol::reply_characters::decode(data);

            keycap::protocol::server_char_enum answer;
            answer.data = reply.characters;

            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[character_handler] sending {}", answer.to_string());

            session->send(answer.encode());

            return true;
        };

        locator_.send_registered(shared_net::account_service_type, request.encode(), get_net_service(), on_reply);
        
        return true;
    }

    bool character_handler::handle_realm_split(keycap::protocol::client_realm_split pakcet)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->trace("[character_handler] handle_realm_split");

        if(pakcet.choice != protocol::realm_split_choice::request)
            return true;

        protocol::server_realm_split answer;
        answer.choice = protocol::realm_split_choice::none;
        answer.state = protocol::realm_split_state::none;
        answer.date = "Soon!";

        session_.send(answer.encode());

        return true;
    }
}