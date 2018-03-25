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

namespace Keycap::Shared::Cli
{
    HandlerResult HandleCommand(std::string const& name, std::vector<std::string> const& arguments,
                                Rbac::Role const& role, CommandMap const& commands)
    {
        if (auto itr = commands.find(name); itr != commands.end())
        {
            auto& command = itr->second;

            if (!role.Has(command.permission))
                return HandlerResult::InsufficientPermissions;

            return command.handler(arguments, role) ? HandlerResult::Ok : HandlerResult::CommandFailed;
        }

        return HandlerResult::CommandNotFound;
    }
}
