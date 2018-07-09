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

#include "Handler.hpp"
#include "../Rbac/Role.hpp"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <iostream>

bool isChild(std::vector<std::string> const& arguments, Keycap::Shared::Cli::Command const& command)
{
    // clang-format off
    return std::any_of(std::begin(command.childCommands), std::end(command.childCommands), [&](auto&& value)
    {
        return std::any_of(std::begin(arguments), std::end(arguments), [&](auto&& argument)
        {
            return argument == value.name;
        });
    });
    // clang-format on
}

namespace Keycap::Shared::Cli
{
    HandlerResult DoCommand(Command const& command, std::vector<std::string> const& arguments, Rbac::Role const& role)
    {
        if (!role.Has(command.permission))
            return HandlerResult::InsufficientPermissions;

        if (command.handler)
            return command.handler(arguments, role) ? HandlerResult::Ok : HandlerResult::CommandFailed;

        std::cout << "Sub commands (name - required permission - description):\n";
        for (auto&& child : command.childCommands)
            std::cout << fmt::format("{} - {} - {}\n", child.name, child.permission.to_string(), child.description);

        return HandlerResult::Ok;
    }

    HandlerResult HandleChildCommand(std::string const& name, std::vector<std::string> const& arguments,
                                     Rbac::Role const& role, Command const& parent);

    HandlerResult HandleCommand(Command const& command, std::string const& name,
                                std::vector<std::string> const& arguments, Rbac::Role const& role)
    {
        if (!isChild(arguments, command))
            return DoCommand(command, arguments, role);

        auto newName = arguments[0];
        auto newArguments = std::vector<std::string>{std::begin(arguments) + 1, std::end(arguments)};

        return HandleChildCommand(newName, newArguments, role, command);
    }

    HandlerResult HandleChildCommand(std::string const& name, std::vector<std::string> const& arguments,
                                     Rbac::Role const& role, Command const& parent)
    {
        auto& childen = parent.childCommands;
        auto command
            = std::find_if(std::begin(childen), std::end(childen), [&](auto&& child) { return child.name == name; });

        if (command != childen.end())
            return HandleCommand(*command, name, arguments, role);

        return HandlerResult::CommandNotFound;
    }

    HandlerResult HandleCommand(std::string const& name, std::vector<std::string> const& arguments,
                                Rbac::Role const& role, CommandMap const& commands)
    {
        if (auto itr = commands.find(name); itr != commands.end())
            return HandleCommand(itr->second, name, arguments, role);

        return HandlerResult::CommandNotFound;
    }
}
