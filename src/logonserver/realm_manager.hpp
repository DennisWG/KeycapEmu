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

#include <generated/shared_protocol.hpp>

#include <unordered_map>

namespace keycap::logonserver
{
    // Stores all realms that have made themselves known to us
    class realm_manager
    {
      public:
        // Inserts the given realm_data to the realm_manager.
        // Will set the realm to online if it has been added before.
        void insert(keycap::protocol::realm_info const& realm_data);

        // Removes the given realm_data from the realm_manager if it has been added
        void remove(keycap::protocol::realm_info const& realm_data);

        // Sets the realm as offline
        void set_offline(uint8_t id);

        // Removes a realm with the given id if it has been added
        void remove(uint8_t id);

        // Iterates all stored realms. Requires callback with signature
        // `void(keycap::shared::network::realm_info const& realm_data)`
        template <typename FUN>
        void iterate(FUN&& callback) const
        {
            for (auto&& [id, realm] : realms_)
                callback(realm);
        }

      private:
        std::unordered_map<uint8_t, keycap::protocol::realm_info> realms_;
    };
}