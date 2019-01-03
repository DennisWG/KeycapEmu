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

#include "handler.hpp"
#include "player_session.hpp"

#include "client_connection.hpp"

#include <keycap/root/network/memory_stream.hpp>

static keycap::realmserver::handler_container handlers{
    // std::make_pair(55, keycap::realmserver::command_handler{keycap::protocol::client_command::char_enum,
    // handle_char_enum}),
};

namespace keycap::realmserver
{
    namespace handler::impl
    {
        template <>
        char const* extract(player_session& session, keycap::root::network::memory_stream& stream)
        {
            return "";
        }

        template <>
        keycap::root::network::memory_stream& extract(player_session& session,
                                                      keycap::root::network::memory_stream& stream)
        {
            return stream;
        }

        template <>
        player_session& extract(player_session& session, keycap::root::network::memory_stream& stream)
        {
            return session;
        }
    }

    handler_container& get_handlers()
    {
        return handlers;
    }
}