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

#include "handler.hpp"
#include "../rbac/role.hpp"

#include <keycap/root/utility/string.hpp>
#include <keycap/root/utility/utility.hpp>

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>

namespace utility = keycap::root::utility;

bool is_child(std::vector<std::string> const& arguments, keycap::shared::cli::command const& command)
{
    // clang-format off
    return std::any_of(std::begin(command.child_commands), std::end(command.child_commands), [&](auto&& value)
    {
        return std::any_of(std::begin(arguments), std::end(arguments), [&](auto&& argument)
        {
            return argument == value.name;
        });
    });
    // clang-format on
}

namespace keycap::shared::cli
{
    handler_result do_command(command const& command, std::vector<std::string> const& arguments, rbac::role const& role)
    {
        auto logger = utility::get_safe_logger("command");
        logger->info("User {} used command '{}' with arguments '{}'.", role.name, command.name,
                     utility::join(arguments));

        if (!role.has(command.permission))
            return handler_result::InsufficientPermissions;

        if (command.handler)
            return command.handler(arguments, role) ? handler_result::Ok : handler_result::CommandFailed;

        std::cout << "Sub commands (name - required permission - description):\n";
        for (auto&& child : command.child_commands)
            std::cout << fmt::format("{} - {} - {}\n", child.name, child.permission.to_string(), child.description);

        return handler_result::Ok;
    }

    handler_result handle_child_command(std::string const& name, std::vector<std::string> const& arguments,
                                        rbac::role const& role, command const& parent);

    handler_result handle_command(command const& command, std::string const& name,
                                  std::vector<std::string> const& arguments, rbac::role const& role)
    {
        if (!is_child(arguments, command))
            return do_command(command, arguments, role);

        auto newName = arguments[0];
        auto newArguments = std::vector<std::string>{std::begin(arguments) + 1, std::end(arguments)};

        return handle_child_command(newName, newArguments, role, command);
    }

    handler_result handle_child_command(std::string const& name, std::vector<std::string> const& arguments,
                                        rbac::role const& role, command const& parent)
    {
        auto& childen = parent.child_commands;
        auto command
            = std::find_if(std::begin(childen), std::end(childen), [&](auto&& child) { return child.name == name; });

        if (command != childen.end())
            return handle_command(*command, name, arguments, role);

        return handler_result::CommandNotFound;
    }

    handler_result handle_command(std::string const& name, std::vector<std::string> const& arguments,
                                  rbac::role const& role, command_map const& commands)
    {
        if (auto itr = commands.find(name); itr != commands.end())
            return handle_command(itr->second, name, arguments, role);

        return handler_result::CommandNotFound;
    }
}
