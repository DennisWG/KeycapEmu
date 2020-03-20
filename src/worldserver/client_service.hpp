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

#include <network/services.hpp>

#include <keycap/root/network/service.hpp>

#include <memory>

namespace keycap::worldserver
{
    class client_connection;

    class client_service : public keycap::root::network::service<client_connection>
    {
      public:
        explicit client_service(int thread_count)
          : service{keycap::root::network::service_mode::Server, shared::network::world_service_type, thread_count}
        {
        }

        virtual bool on_new_connection(SharedHandler handler) override;

        virtual SharedHandler make_handler(boost::asio::ip::tcp::socket socket) override;
    };
}