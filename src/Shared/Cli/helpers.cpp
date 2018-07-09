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

#include "Helpers.hpp"

#include <keycap/root/utility/scope_exit.hpp>

#include <iostream>


extern Keycap::Shared::Cli::CommandMap& GetCommandMap();

namespace Keycap::Shared::Cli
{
    void ExtractCommandAndArguments(std::string const& line, std::string& outCommand, std::vector<std::string>& outArgs)
    {
        auto tokens = keycap::root::utility::explode(line);
        outCommand = tokens[0];

        std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(outArgs));

        outArgs.erase(std::remove_if(std::begin(outArgs), std::end(outArgs), [](auto& i) { return i.empty(); }),
                      std::end(outArgs));
    }

    void PrintHandlerResult(Keycap::Shared::Cli::HandlerResult result, std::string const& command)
    {
        switch (result)
        {
            case Keycap::Shared::Cli::HandlerResult::Ok:
                break;
            case Keycap::Shared::Cli::HandlerResult::InsufficientPermissions:
            case Keycap::Shared::Cli::HandlerResult::CommandNotFound:
                fmt::print("Unknown command '{}'\n", command);
                break;
            case Keycap::Shared::Cli::HandlerResult::CommandFailed:
                fmt::print("Command '{}' failed to execute!\n", command);
                break;
        }
    }

    void RunCommandLine(Keycap::Shared::Rbac::Role& consoleRole, bool& running)
    {
        std::cout << '>';
        for (std::string line; std::getline(std::cin, line);)
        {
            QUICK_SCOPE_EXIT(sc, []() { std::cout << '>'; });

            if (line.empty())
                continue;

            std::string command;
            std::vector<std::string> arguments;

            Keycap::Shared::Cli::ExtractCommandAndArguments(line, command, arguments);

            auto result = Keycap::Shared::Cli::HandleCommand(command, arguments, consoleRole, GetCommandMap());
            PrintHandlerResult(result, command);

            if (!running)
                break;
        }
    }
}