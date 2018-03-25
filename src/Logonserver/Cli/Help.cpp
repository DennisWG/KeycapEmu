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

namespace cli = Keycap::Shared::Cli;
namespace rbac = Keycap::Shared::Rbac;

extern cli::CommandMap& GetCommandMap();

namespace Keycap::Logonserver::Cli
{
    bool HelpCommand(std::vector<std::string> const& args, rbac::Role const& role)
    {
        if (args.empty())
        {
            for (auto[name, command] : GetCommandMap())
            {
                if(role.Has(command.permission))
                    std::cout << name << " - " << command.description << '\n';
            }

            return true;
        }

        return false;
    }

    cli::Command RegisterHelp()
    {
        using Keycap::Shared::Permission;
        using namespace std::string_literals;

        return cli::Command{"help"s, Permission::CommandHelp, &HelpCommand,
                            "Displays all commands or details about a specific command"s};
    }
}
