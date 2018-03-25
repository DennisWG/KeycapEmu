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

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Keycap::Shared::Rbac
{
    struct Role;
}

namespace Keycap::Shared::Cli
{
    class Command final
    {
      public:
        using Callback = std::function<bool(std::vector<std::string> const& arguments, Rbac::Role const& role)>;

        std::string name;
        Keycap::Shared::Permission permission;
        Callback handler;
        std::string description;
        std::vector<Command> childCommands;

        Command(std::string const& name, Keycap::Shared::Permission permission, Callback handler,
                std::string const& description = "", std::vector<Command> const& childCommands = {})
          : name(name)
          , permission(permission)
          , handler(handler)
          , description(description)
          , childCommands(childCommands)
        {
        }

        Command() = default;
    };

    using CommandMap = std::unordered_map<std::string, Command>;
}
