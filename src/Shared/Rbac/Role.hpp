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

#pragma once

#include <Permissions.hpp>
#include <Keycap/Root/Types.hpp>

#include <unordered_set>

namespace Keycap::Shared::Rbac
{
    using PermissionSet = std::unordered_set<Shared::Permission, std::hash<uint32>, std::equal_to<uint32>>;

    // A Role contains a set of permissions
    struct Role final
    {
        uint32 id = 0;
        std::string name = "";
        PermissionSet permissions;

        void Add(Shared::Permission perm)
        {
            permissions.insert(perm);
        }

        void Remove(Shared::Permission perm)
        {
            permissions.erase(perm);
        }

        bool Has(Shared::Permission perm) const
        {
            return permissions.find(perm) != permissions.end();
        }
    };
}