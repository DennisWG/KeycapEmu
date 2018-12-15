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

#include <network/state_result.hpp>

#include <botan/bigint.h>

#include <keycap/root/network/data_router.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/message_handler.hpp>
#include <keycap/root/network/service_connection.hpp>
#include <keycap/root/network/service_type.hpp>
#include <keycap/root/network/srp6/server.hpp>

#include <variant>

namespace keycap::shared::network
{
    class request_account_data;
    class update_session_key;

    class request_session_key;
}

namespace keycap::accountserver
{
    class connection : public keycap::root::network::service_connection
    {
      public:
        explicit connection(keycap::root::network::service_base& service);

        bool on_data(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     uint64 sender, keycap::root::network::memory_stream& stream) override;

        bool on_link(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     keycap::root::network::link_status status) override;

      private:
        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            shared::network::state_result on_data(connection& connection, uint64 sender,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct connected
        {
            shared::network::state_result on_data(connection& connection, uint64 sender,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "JustConnected";

          private:
            shared::network::state_result
            on_account_data_request(std::weak_ptr<accountserver::connection>& connection_ptr, uint64 sender,
                                    shared::network::request_account_data& packet);

            shared::network::state_result
            on_update_session_key(std::weak_ptr<accountserver::connection>& connection_ptr, uint64 sender,
                                  shared::network::update_session_key& packet);

            shared::network::state_result
            on_session_key_request(std::weak_ptr<accountserver::connection>& connection_ptr, uint64 sender,
                                   shared::network::request_session_key& packet);
        };

        std::variant<disconnected, connected> state_;

        keycap::root::network::memory_stream input_stream_;
    };
}