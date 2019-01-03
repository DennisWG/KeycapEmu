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

#include <cli/command.hpp>
#include <generated/permissions.hpp>
#include <rbac/role.hpp>

#include <iostream>

namespace rbac = keycap::shared::rbac;

extern keycap::shared::cli::command_map& get_command_map();

namespace keycap::accountserver::cli
{
    bool help_command(std::vector<std::string> const& args, rbac::role const& role)
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

    keycap::shared::cli::command register_help()
    {
        using keycap::shared::permission;
        using namespace std::string_literals;

        return keycap::shared::cli::command{"help"s, permission::CommandHelp, &help_command,
                                            "Displays all commands or details about a specific command"s};
    }
}
