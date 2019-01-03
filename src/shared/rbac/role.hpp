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

#include <generated/permissions.hpp>
#include <keycap/root/types.hpp>

#include <unordered_set>

namespace keycap::shared::rbac
{
    using permission_set = std::unordered_set<shared::permission, std::hash<uint32>, std::equal_to<uint32>>;

    // A Role contains a set of permissions
    struct role final
    {
        uint32 id = 0;
        std::string name = "";
        permission_set permissions;

        // Adds the given permission
        void add(shared::permission perm)
        {
            permissions.insert(perm);
        }

        // Removes the given permission
        void remove(shared::permission perm)
        {
            permissions.erase(perm);
        }

        // Returns if the given permission has been added to the role
        bool has(shared::permission perm) const
        {
            return permissions.find(perm) != permissions.end();
        }
    };
}