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

#include <cli/command.hpp>

namespace keycap::logonserver::cli
{
    namespace cli = keycap::shared::cli;
    extern cli::command register_help();
    extern cli::command register_account();

    namespace impl
    {
        void register_command(cli::command const& command, cli::command_map& commandMap)
        {
            commandMap[command.name] = command;
        }
    }

    void register_commands(cli::command_map& commandMap)
    {
        impl::register_command(register_help(), commandMap);
        impl::register_command(register_account(), commandMap);
    }
}