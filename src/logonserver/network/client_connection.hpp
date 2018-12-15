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

#include "logon.hpp"
#include "logon_service.hpp"

#include <network/state_result.hpp>

#include <botan/bigint.h>

#include <keycap/root/network/connection.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/message_handler.hpp>
#include <keycap/root/network/service_locator.hpp>
#include <keycap/root/network/srp6/server.hpp>

#include <variant>

namespace keycap::shared::network
{
    class reply_account_data;
}

namespace keycap::logonserver
{
    class client_connection : public keycap::root::network::connection, public keycap::root::network::message_handler
    {
        using base_connection = keycap::root::network::connection;

      public:
        client_connection(keycap::root::network::service_base& service,
                          keycap::root::network::service_locator& locator);

        bool on_data(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     std::vector<uint8_t> const& data) override;

        bool on_link(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     keycap::root::network::link_status status) override;

        keycap::root::network::service_locator& service_locator()
        {
            return locator_;
        }

        void send_error(protocol::grunt_result result);

      private:
        struct challanged_data
        {
            std::shared_ptr<keycap::root::network::srp6::server> server;
            Botan::BigInt v;
            keycap::root::network::srp6::compliance compliance;

            std::string username;
            Botan::BigInt user_salt;
            Botan::secure_vector<uint8_t> checksum_salt;
            protocol::account_flag account_flags;
        };

        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            shared::network::state_result on_data(client_connection& connection,
                                                  keycap::root::network::data_router const& router,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct just_connected
        {
            shared::network::state_result on_data(client_connection& connection,
                                                  keycap::root::network::data_router const& router,
                                                  keycap::root::network::memory_stream& stream);

            void on_account_reply(std::weak_ptr<client_connection> connection,
                                  shared::network::reply_account_data& reply, std::string const& account_name);

            std::string name = "JustConnected";

          private:
            void send_server_challange(std::shared_ptr<client_connection> conn, challanged_data const& challanged_data,
                                       keycap::root::network::srp6::compliance compliance,
                                       keycap::root::network::srp6::group_parameter const& parameter,
                                       Botan::BigInt const& salt, uint8 security_options);
        };

        // Auth Challange was send to client and we're waiting for a response
        struct challanged
        {
            challanged() = default;
            explicit challanged(challanged_data const& data)
              : data{data}
            {
            }
            shared::network::state_result on_data(client_connection& connection,
                                                  keycap::root::network::data_router const& router,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Challanged";
            challanged_data data;

          private:
            std::tuple<Botan::BigInt, Botan::BigInt, Botan::BigInt>
            generate_session_key_and_server_proof(protocol::client_logon_proof const& packet);

            void send_proof_success(client_connection& connection, Botan::BigInt session_key,
                                    Botan::BigInt M1_S);
        };

        // Client send its Challange and is now authenticated
        struct authenticated
        {
            shared::network::state_result on_data(client_connection& connection,
                                                  keycap::root::network::data_router const& router,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Authenticated";
        };

        std::variant<disconnected, just_connected, challanged, authenticated> state_;

        keycap::root::network::memory_stream input_stream_;

        keycap::root::network::service_locator& locator_;
    };
}