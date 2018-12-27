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

#include "realm_service.hpp"

namespace keycap::logonserver
{
    realm_service::realm_service(int thread_count, realm_manager& realm_manager)
      : service{keycap::root::network::service_mode::Server, shared::network::logon_realm_service_type, thread_count}
      , realm_manager_{realm_manager}
    {
    }

    bool realm_service::on_new_connection(SharedHandler handler)
    {
        return true;
    }

    realm_service::SharedHandler realm_service::make_handler()
    {
        return std::make_shared<realm_connection>(*this, realm_manager_);
    }
}