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

#include "login_queue.hpp"

#include <generated/client.hpp>
#include <generated/shared_protocol.hpp>

#include <cryptography/packet_scrambler.hpp>
#include <network/state_result.hpp>

#include <keycap/root/network/connection.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/message_handler.hpp>

#include <variant>

namespace keycap::root::network
{
    class service_locator;
}

namespace keycap::protocol
{
    class reply_session_key;
    class client_addon_info;
}

namespace Botan
{
    class BigInt;
}

namespace keycap::realmserver
{
    class player_session;

    class client_connection : public keycap::root::network::connection, public keycap::root::network::message_handler
    {
      public:
        client_connection(keycap::root::network::service_base& service,
                          keycap::root::network::service_locator& locator);

        bool on_data(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     std::vector<uint8_t> const& data) override;

        bool on_link(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     keycap::root::network::link_status status) override;

        uint32_t auth_seed() const;

        keycap::root::network::service_locator& locator() const;

      private:
        using state_result = std::tuple<shared::network::state_result, uint16, keycap::protocol::client_command>;

        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            state_result on_data(client_connection& connection, keycap::root::network::data_router const& router,
                                 keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established. We're not authed yet
        struct just_connected
        {
          private:
            struct account_reply_data
            {
                std::string const& account_name;
                uint32 client_seed;
                std::array<uint8, 20> const& digest;
                root::network::memory_stream addon_data;
            };

          public:
            just_connected(client_connection& connection);
            state_result on_data(client_connection& connection, keycap::root::network::data_router const& router,
                                 keycap::root::network::memory_stream& stream);

            void on_account_reply(std::weak_ptr<client_connection> connection,
                                  keycap::protocol::reply_session_key& reply, account_reply_data& data);

            bool verify_digest(std::string const& account_name, uint32 client_seed, uint32 auth_seed,
                               Botan::BigInt const& session_key, std::array<uint8, 20> const& client_digest);

            std::string name = "JustConnected";
        };

        // The client successfully authenticated itself
        struct authenticated
        {
          public:
            authenticated(std::shared_ptr<client_connection> connection,
                          keycap::protocol::client_addon_info const& client_addons, Botan::BigInt const& session_key);

            state_result on_data(client_connection& connection, keycap::root::network::data_router const& router,
                                 keycap::root::network::memory_stream& stream);

            std::string name = "authenticated";
        };

        std::string account_name;

        std::variant<disconnected, just_connected, authenticated> state_;

        uint32_t auth_seed_ = 0;
        keycap::root::network::memory_stream input_stream_;

        keycap::root::network::service_locator& locator_;

        shared::cryptography::packet_scrambler scrambler_;
        login_queue login_queue_;

        std::unique_ptr<player_session> player_session_;
    };
}