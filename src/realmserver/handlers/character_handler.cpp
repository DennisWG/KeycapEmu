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

#include "character_handler.hpp"

#include "../network/player_session.hpp"
#include "../network/handler.hpp"

#include <spdlog/spdlog.h>

namespace keycap::realmserver
{
    character_handler::character_handler(player_session& session)
      : session_{session}
    {
        auto& handlers = get_handlers();

        keycap_add_handler(keycap::protocol::client_command::char_enum, this, &character_handler::handle_char_enum);
        keycap_add_handler(keycap::protocol::client_command::realm_split, this, &character_handler::handle_realm_split);
    }

    bool character_handler::handle_char_enum(keycap::protocol::client_char_enum packet)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->trace("[character_handler] handle_char_enum");

        keycap::protocol::char_data data{
            0,         //   guid;
            "Testzor", //   name;
            1,         //  race;
            2,         //  player_class;
            1,         //  gender;
            1,         //  skin;
            1,         //  face;
            1,         //  hair_style;
            1,         //  hair_color;
            1,         //  facial_hair;
            41,        //  level;
            1,         //   zone;
            1,         //  map;
            0.f,       //  x;
            0.f,       //  y;
            0.f,       //  z;
            0,         //   guild_id;
            0,         //   flags;
            1,         //  first_login;
            0,         //   pet_display_id;
            0,         //   pet_level;
            0,         //   pet_family;
            {}         //
        };

        keycap::protocol::server_char_enum answer;
        //answer.data.emplace_back(std::move(data));

        logger->debug("[character_handler] sending {}", answer.to_string());

        auto foo = answer.encode();
        //foo.put<uint8>(0);
        //session_.send(answer.encode());
        session_.send(foo);

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