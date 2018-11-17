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

#include "role.hpp"

namespace keycap::shared::rbac
{
    permission_set get_all_permissions()
    {
        auto const& perms = permission::to_vector();
        return permission_set{std::begin(perms), std::end(perms)};
    }
}