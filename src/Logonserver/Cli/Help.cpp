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

#include <Cli/Command.hpp>
#include <Permissions.hpp>
#include <Rbac/Role.hpp>

#include <iostream>

namespace cli = keycap::shared::cli;
namespace rbac = keycap::shared::rbac;

extern cli::command_map& get_command_map();

namespace Keycap::Logonserver::Cli
{
    bool HelpCommand(std::vector<std::string> const& args, rbac::role const& role)
    {
        if (args.empty())
        {
            for (auto[name, command] : get_command_map())
            {
                if (role.has(command.permission))
                    std::cout << name << " - " << command.description << '\n';
            }

            return true;
        }

        return false;
    }

    cli::command RegisterHelp()
    {
        using keycap::shared::permission;
        using namespace std::string_literals;

        return cli::command{"help"s, permission::CommandHelp, &HelpCommand,
                            "Displays all commands or details about a specific command"s};
    }
}
