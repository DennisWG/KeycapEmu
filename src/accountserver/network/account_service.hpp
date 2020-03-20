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

namespace keycap::accountserver
{
    class connection;
    class character_id_provider;

    class account_service : public keycap::root::network::service<connection>
    {
      public:
        explicit account_service(int thread_count, character_id_provider& character_id_provider)
          : service{keycap::root::network::service_mode::Server, shared::network::account_service_type, thread_count}
          , character_id_provider_{character_id_provider}
        {
        }

      protected:
        virtual SharedHandler make_handler(boost::asio::ip::tcp::socket socket) override
        {
            return std::make_shared<connection>(std::move(socket), *this, character_id_provider_);
        }

        character_id_provider& character_id_provider_;
    };
}