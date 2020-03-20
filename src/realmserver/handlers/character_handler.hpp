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

#pragma once

#include <generated/character_select.hpp>
#include <generated/realm_protocol.hpp>

namespace keycap::root::network
{
    class service_locator;
}

namespace keycap::realmserver
{
    class player_session;

    class character_handler
    {
      public:
        explicit character_handler(player_session& session, keycap::root::network::service_locator& locator);

        bool handle_char_create(keycap::protocol::client_char_create packet);
        bool handle_char_enum(keycap::protocol::client_char_enum packet);

        bool handle_realm_split(keycap::protocol::client_realm_split pakcet);

      private:
        player_session& session_;

        keycap::root::network::service_locator& locator_;
    };
}