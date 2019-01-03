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
#include <keycap/root/network/service_locator.hpp>

#include <memory>

namespace keycap::logonserver
{
    class client_connection;
    class realm_manager;

    class logon_service : public keycap::root::network::service<client_connection>
    {
      public:
        logon_service(int thread_count, keycap::root::network::service_locator& locator, realm_manager& realm_manager)
          : service{keycap::root::network::service_mode::Server, shared::network::logon_service_type, thread_count}
          , locator_{locator}
          , realm_manager_{realm_manager}
        {
        }

        virtual bool on_new_connection(SharedHandler handler) override;

        virtual SharedHandler make_handler() override;

      private:
        keycap::root::network::service_locator& locator_;
        realm_manager& realm_manager_;
    };
}