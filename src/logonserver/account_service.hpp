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

#include <network/services.hpp>

#include <keycap/root/network/service.hpp>

namespace keycap::logonserver
{
    class account_connection;

    class account_service final : public keycap::root::network::service<account_connection>
    {
      public:
        account_service()
          : service{keycap::root::network::service_mode::Client, shared::network::account_service, 1}
        {
        }

      protected:
        virtual SharedHandler make_handler() override
        {
            return std::make_shared<account_connection>(*this);
        }
    };
}