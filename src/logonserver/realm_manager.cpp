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

#include "realm_manager.hpp"

namespace keycap::logonserver
{
    void realm_manager::insert(keycap::protocol::realm_info const& realm_data)
    {
        if (auto const& realm = realms_.find(realm_data.id); realm != realms_.end())
            realm->second.realm_flags.clear_flag(keycap::protocol::realm_flag::offline);
        else
            realms_[realm_data.id] = realm_data;
    }

    void realm_manager::remove(keycap::protocol::realm_info const& realm_data)
    {
        realms_.erase(realm_data.id);
    }

    void realm_manager::set_offline(uint8_t id)
    {
        realms_[id].realm_flags.set_flag(keycap::protocol::realm_flag::offline);
    }

    void realm_manager::remove(uint8_t id)
    {
        realms_.erase(id);
    }
}