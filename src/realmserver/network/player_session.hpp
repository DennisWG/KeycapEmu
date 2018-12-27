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

#pragma once

namespace keycap
{
    namespace root::network
    {
        class memory_stream;
    }

    namespace protocol
    {
        class client_addon_info;
    }

    namespace shared::cryptography
    {
        class packet_scrambler;
    }
}

namespace Botan
{
    class BigInt;
}

namespace keycap::realmserver
{
    class client_connection;

    class player_session
    {
      public:
        player_session(client_connection& connection, shared::cryptography::packet_scrambler& scrambler);

        void send_addon_info(keycap::protocol::client_addon_info const& client_addons);

        void send(keycap::root::network::memory_stream& stream);
        void send(keycap::root::network::memory_stream&& stream);

      private:
        client_connection& connection_;
        shared::cryptography::packet_scrambler& scrambler_;
    };
}