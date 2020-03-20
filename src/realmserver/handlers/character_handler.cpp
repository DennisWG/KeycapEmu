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

#include "../network/handler.hpp"
#include "../network/player_session.hpp"

#include <generated/shared_protocol.hpp>

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
        keycap_add_handler(keycap::protocol::client_command::char_create, this, &character_handler::handle_char_create);
    }

    bool character_handler::handle_char_create(keycap::protocol::client_char_create packet)
    {
        keycap::protocol::char_create request;
        keycap::protocol::char_data data{
            0,                   // uint64 guid; - created by the account server
            packet.name,         // string name;
            packet.race,         // uint8 race;
            packet.player_class, // uint8 player_class;
            packet.gender,       // uint8 gender;
            packet.skin,         // uint8 skin;
            packet.face,         // uint8 face;
            packet.hair_style,   // uint8 hair_style;
            packet.hair_color,   // uint8 hair_color;
            packet.facial_hair,  // uint8 facial_hair;
            1,                   // uint8 level;
            0,                   // uint32 zone;
            0,                   // uint32 map;
            0,                   // float x;
            0,                   // float y;
            0,                   // float z;
            0,                   // uint32 guild_id;
            0,                   // uint32 flags;
            0,                   // uint8 first_login;
            0,                   // uint32 pet_display_id;
            0,                   // uint32 pet_level;
            0,                   // uint32 pet_family;
            // char_item_data[20] items;
        };

        request.realm_id = get_realm_id();
        request.account_id = session_.account_id();
        request.data = data;

        auto on_reply = [session = &session_](net::service_type sender, net::memory_stream data) {
            if (data.peek<keycap::protocol::shared_command>() != keycap::protocol::shared_command::reply_char_create)
                return false;

            auto reply = keycap::protocol::reply_char_create::decode(data);

            keycap::protocol::server_char_create answer;
            answer.result = reply.result;

            session->send(answer.encode());

            return true;
        };

        locator_.send_registered(shared_net::account_service_type, request.encode(), get_net_service(), on_reply);

        /*
        static uint8 result = 47;

        keycap::root::network::memory_stream encoder;
        auto position_size = static_cast<uint16>(encoder.size());
        encoder.put<uint16>(0);
        encoder.put(keycap::protocol::server_command::char_create);
        encoder.put<uint8>(result);

        auto position_size_value
            = std::max<uint16>(0, static_cast<uint16>(encoder.size() - sizeof(uint16) - position_size));
        encoder.override(boost::endian::endian_reverse(position_size_value), position_size);

        session_.send(encoder);
        //*/

        return false;
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

        if (pakcet.choice != protocol::realm_split_choice::request)
            return true;

        protocol::server_realm_split answer;
        answer.choice = protocol::realm_split_choice::none;
        answer.state = protocol::realm_split_state::none;
        answer.date = "Soon!";

        session_.send(answer.encode());

        return true;
    }
}