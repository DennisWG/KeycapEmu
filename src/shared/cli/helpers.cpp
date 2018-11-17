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

#include "helpers.hpp"

#include <keycap/root/utility/scope_exit.hpp>

#include <iostream>

extern keycap::shared::cli::command_map& get_command_map();

namespace keycap::shared::cli
{
    void extract_command_and_arguments(std::string const& line, std::string& outCommand,
                                       std::vector<std::string>& outArgs)
    {
        auto tokens = keycap::root::utility::explode(line);
        outCommand = tokens[0];

        std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(outArgs));

        outArgs.erase(std::remove_if(std::begin(outArgs), std::end(outArgs), [](auto& i) { return i.empty(); }),
                      std::end(outArgs));
    }

    void print_handler_result(keycap::shared::cli::handler_result result, std::string const& command)
    {
        switch (result)
        {
            case keycap::shared::cli::handler_result::Ok:
                break;
            case keycap::shared::cli::handler_result::InsufficientPermissions:
            case keycap::shared::cli::handler_result::CommandNotFound:
                fmt::print("Unknown command '{}'\n", command);
                break;
            case keycap::shared::cli::handler_result::CommandFailed:
                fmt::print("Command '{}' failed to execute!\n", command);
                break;
        }
    }

    void run_command_line(keycap::shared::rbac::role& consoleRole, bool& running)
    {
        std::cout << '>';
        for (std::string line; std::getline(std::cin, line);)
        {
            QUICK_SCOPE_EXIT(sc, []() { std::cout << '>'; });

            if (line.empty())
                continue;

            std::string command;
            std::vector<std::string> arguments;

            keycap::shared::cli::extract_command_and_arguments(line, command, arguments);

            auto result = keycap::shared::cli::handle_command(command, arguments, consoleRole, get_command_map());
            print_handler_result(result, command);

            if (!running)
                break;
        }
    }

    void run_command_line(keycap::shared::rbac::role&& consoleRole, bool& running)
    {
        run_command_line(consoleRole, running);
    }
}