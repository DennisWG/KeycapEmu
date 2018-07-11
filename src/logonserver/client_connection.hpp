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

#include <botan/bigint.h>

#include <keycap/root/network/connection.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/message_handler.hpp>
#include <keycap/root/network/srp6/server.hpp>

#include <variant>

namespace keycap::logonserver
{
    class client_connection : public keycap::root::network::connection<client_connection>,
                              public keycap::root::network::message_handler
    {
        using base_connection = keycap::root::network::connection<client_connection>;

      public:
        explicit client_connection(keycap::root::network::service_base& service);

        bool on_data(keycap::root::network::service_base& service, std::vector<uint8_t> const& data) override;

        bool on_link(keycap::root::network::service_base& service, keycap::root::network::link_status status) override;

      private:
        enum class state_result
        {
            // We've received the packet as intended and are ready to move on to the next state
            Ok,
            // There was some kind of error in the received packet and we have to terminate the connection
            Abort,
            // We're still wating for more data to arrive from the client
            IncompleteData,
        };

        struct challanged_data
        {
            std::shared_ptr<keycap::root::network::srp6::server> server;
            Botan::BigInt v;
            keycap::root::network::srp6::compliance compliance;

            std::string username;
            Botan::BigInt user_salt;
            Botan::secure_vector<uint8_t> checksum_salt;
        };

        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            state_result on_data(client_connection& connection, keycap::root::network::service_base& service,
                                keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct just_connected
        {
            state_result on_data(client_connection& connection, keycap::root::network::service_base& service,
                                keycap::root::network::memory_stream& stream);

            std::string name = "JustConnected";
        };

        // Auth Challange was send to client and we're waiting for a response
        struct challanged
        {
            challanged() = default;
            explicit challanged(challanged_data const& data)
              : data{data}
            {
            }
            state_result on_data(client_connection& connection, keycap::root::network::service_base& service,
                                keycap::root::network::memory_stream& stream);

            std::string name = "Challanged";
            challanged_data data;
        };

        // Client send its Challange and is now authenticated
        struct authenticated
        {
            state_result on_data(client_connection& connection, keycap::root::network::service_base& service,
                                keycap::root::network::memory_stream& stream);

            std::string name = "Authenticated";
        };

        std::variant<disconnected, just_connected, challanged, authenticated> state_;

        keycap::root::network::memory_stream input_stream_;
    };
}