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

#include "../realm_manager.hpp"

#include <network/state_result.hpp>

#include <keycap/root/network/service_connection.hpp>

#include <variant>

namespace keycap::logonserver
{
    class realm_connection : public keycap::root::network::service_connection
    {
      public:
        explicit realm_connection(keycap::root::network::service_base& service, realm_manager& realm_manager);

        bool on_data(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     uint64 sender, keycap::root::network::memory_stream& stream) override;

        bool on_link(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     keycap::root::network::link_status status) override;

      private:
        // Connection hasn't been established yet or has been terminated
        struct disconnected
        {
            shared::network::state_result on_data(realm_connection& connection, uint64 sender,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct connected
        {
            shared::network::state_result on_data(realm_connection& connection, uint64 sender,
                                                  keycap::root::network::memory_stream& stream);

            std::string name = "JustConnected";

          private:
        };
        friend struct connected;

        std::variant<disconnected, connected> state_;

        realm_manager& realm_manager_;

        uint8_t realm_id_ = 0;
    };
} // namespace keycap::logonserver