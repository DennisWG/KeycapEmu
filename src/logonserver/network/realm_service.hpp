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

#include "realm_connection.hpp"

#include <network/services.hpp>

#include <keycap/root/network/service.hpp>

namespace keycap::logonserver
{
    class realm_manager;

    class realm_service : public keycap::root::network::service<realm_connection>
    {
      public:
        realm_service(int thread_count, realm_manager& realm_manager);

        virtual bool on_new_connection(SharedHandler handler) override;

        virtual SharedHandler make_handler(boost::asio::ip::tcp::socket socket) override;

      private:
        realm_manager& realm_manager_;
    };
}