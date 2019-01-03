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

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace keycap::shared::rbac
{
    struct role;
}

namespace keycap::shared::cli
{
    class command final
    {
      public:
        using callback = std::function<bool(std::vector<std::string> const& arguments, rbac::role const& role)>;

        std::string name;
        keycap::shared::permission permission;
        callback handler;
        std::string description;
        std::vector<command> child_commands;

        command(std::string const& name, keycap::shared::permission permission, callback handler,
                std::string const& description = "", std::vector<command> const& child_commands = {})
          : name(name)
          , permission(permission)
          , handler(handler)
          , description(description)
          , child_commands(child_commands)
        {
        }

        command() = default;
    };

    using command_map = std::unordered_map<std::string, command>;
}
