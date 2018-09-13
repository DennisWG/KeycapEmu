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

#include "account_service.hpp"

#include <botan/bigint.h>

#include <keycap/root/network/data_router.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/message_handler.hpp>
#include <keycap/root/network/service_connection.hpp>
#include <keycap/root/network/srp6/server.hpp>

#include <variant>

namespace keycap::accountserver
{
    class connection : public keycap::root::network::service_connection
    {
      public:
        explicit connection(keycap::root::network::service_base& service);

        bool on_data(keycap::root::network::data_router const& router, uint64 sender,
                     keycap::root::network::memory_stream& stream) override;

        bool on_link(keycap::root::network::data_router const& router,
                     keycap::root::network::link_status status) override;

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

        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            state_result on_data(connection& connection, uint64 sender, keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct connected
        {
            state_result on_data(connection& connection, uint64 sender, keycap::root::network::memory_stream& stream);

            std::string name = "JustConnected";
        };

        std::variant<disconnected, connected> state_;

        keycap::root::network::memory_stream input_stream_;
    };
}