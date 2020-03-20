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

#include <keycap/root/network/connection.hpp>
#include <keycap/root/network/message_handler.hpp>

namespace keycap::worldserver
{
    class client_connection : public keycap::root::network::connection, public keycap::root::network::message_handler
    {
      public:
        explicit client_connection(boost::asio::ip::tcp::socket socket, keycap::root::network::service_base& service);

        bool on_data(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     gsl::span<uint8_t> data) override;

        bool on_link(keycap::root::network::data_router const& router, keycap::root::network::service_type service,
                     keycap::root::network::link_status status) override;
    };
}