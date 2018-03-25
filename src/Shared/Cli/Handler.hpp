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

#include "Command.hpp"

namespace Keycap::Shared::Cli
{
    enum class HandlerResult
    {
        Ok,
        CommandNotFound,
        CommandFailed,
        InsufficientPermissions,
    };

    // Executes the Command with the given name and the given argumets if the given role has the permissions the execute
    // it
    HandlerResult HandleCommand(std::string const& name, std::vector<std::string> const& arguments,
                                Rbac::Role const& role, CommandMap const& commands);
}
