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

#include <keycap/root/network/service_locator.hpp>
#include <keycap/root/utility/enum.hpp>

namespace keycap::shared::network
{
    keycap_enum(services, uint32,
                logon = 0,       // user -> logon server
                logon_realm = 1, // realm server -> logon server
                account = 2,     // internal service -> account server
                realm = 3,       // user -> realm server
				world = 4,		 // realm server -> world server
    );

    constexpr static keycap::root::network::service_type logon_service_type{services::logon};
    constexpr static keycap::root::network::service_type logon_realm_service_type{services::logon_realm};
    constexpr static keycap::root::network::service_type account_service_type{services::account};
    constexpr static keycap::root::network::service_type realm_service_type{services::realm};
    constexpr static keycap::root::network::service_type world_service_type{services::world};

    constexpr static services logon_service{services::logon};
    constexpr static services logon_realm_service{services::logon_realm};
    constexpr static services account_service{services::account};
    constexpr static services realm_service{services::realm};
    constexpr static services world_service{services::world};
}