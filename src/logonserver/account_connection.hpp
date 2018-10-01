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

#include <network/state_result.hpp>

#include <keycap/root/network/connection.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/message_handler.hpp>

#include <variant>

namespace keycap::logonserver
{
    class account_connection : public keycap::root::network::connection, public keycap::root::network::message_handler
    {
        using BaseConnection = keycap::root::network::connection;

      public:
        explicit account_connection(keycap::root::network::service_base& service);

        bool on_data(keycap::root::network::data_router const& router, std::vector<uint8_t> const& data) override;

        bool on_link(keycap::root::network::data_router const& router,
                     keycap::root::network::link_status status) override;

      private:
        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            shared::network::state_result on_data(account_connection& connection,
                                                  keycap::root::network::data_router const& router,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct connected
        {
            shared::network::state_result on_data(account_connection& connection,
                                                  keycap::root::network::data_router const& router,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Connected";
        };

        std::variant<disconnected, connected> state_;

        keycap::root::network::memory_stream input_stream_;
    };
}