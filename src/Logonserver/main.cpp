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

#include "Cli/Registrar.hpp"
#include "Connection.hpp"
#include "Version.hpp"
#include <Cli/Handler.hpp>

#include <Keycap/Root/Utility/String.hpp>
#include <Keycap/Root/Utility/Utility.hpp>

#include <Rbac/Role.hpp>

#include <spdlog/spdlog.h>

#include <iostream>

#undef SetConsoleTitle

Keycap::Shared::Cli::CommandMap commands;

void RegisterCommand(Keycap::Shared::Cli::Command const& command)
{
    commands[command.name] = command;
}

void RegisterDefaultCommands(bool& running)
{
    using namespace std::string_literals;
    using Keycap::Shared::Permission;
    using Keycap::Shared::Cli::Command;
    namespace rbac = Keycap::Shared::Rbac;

    RegisterCommand(Command{"shutdown"s, Permission::CommandShutdown,
                            [&running](std::vector<std::string> const& args, rbac::Role const& role) {
                                running = false;
                                return true;
                            },
                            "Shuts down the Server"s});
}

auto& GetCommandMap()
{
    return commands;
}

void CreateLogger(std::string const& name, bool console, bool file, spdlog::level::level_enum level)
{
    std::vector<spdlog::sink_ptr> sinks;
    if (console)
        sinks.emplace_back(std::move(std::make_shared<spdlog::sinks::stdout_sink_mt>()));
    if (file)
        sinks.emplace_back(std::move(std::make_shared<spdlog::sinks::daily_file_sink_mt>(name, 23, 59)));
    auto combined_logger = std::make_shared<spdlog::logger>(name, std::begin(sinks), std::end(sinks));
    combined_logger->set_level(level);
    spdlog::register_logger(combined_logger);
}

void CreateLoggers()
{
    CreateLogger("console", true, true, spdlog::level::debug);
    CreateLogger("command", false, true, spdlog::level::debug);
    CreateLogger("connections", true, true, spdlog::level::debug);
}

Keycap::Shared::Rbac::PermissionSet GetAllPermissions()
{
    auto const& perms = Keycap::Shared::Permission::ToVector();
    return Keycap::Shared::Rbac::PermissionSet{std::begin(perms), std::end(perms)};
}

Keycap::Shared::Rbac::Role CreateConsoleRole()
{
    return Keycap::Shared::Rbac::Role{0, "Console", GetAllPermissions()};
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

void ExtractCommandAndArguments(std::string const& line, std::string& outCommand, std::vector<std::string>& outArgs)
{
    auto tokens = Keycap::Root::Utility::Explode(line);
    outCommand = tokens[0];

    std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(outArgs));

    outArgs.erase(std::remove_if(std::begin(outArgs), std::end(outArgs), [](auto& i) { return i.empty(); }),
                  std::end(outArgs));
}

void RunCommandLine(Keycap::Shared::Rbac::Role& consoleRole, bool& running)
{
    std::cout << '>';
    for (std::string line; std::getline(std::cin, line);)
    {
        struct ScopeExit
        {
            ~ScopeExit()
            {
                std::cout << '>';
            }
        } sc;
        if (line.empty())
            continue;

        std::string command;
        std::vector<std::string> arguments;
        ExtractCommandAndArguments(line, command, arguments);

        auto result = Keycap::Shared::Cli::HandleCommand(command, arguments, consoleRole, commands);
        PrintHandlerResult(result, command);

        if (!running)
            break;
    }
}

int main()
{
    namespace utility = Keycap::Root::Utility;

    auto consoleRole = CreateConsoleRole();

    utility::SetConsoleTitle("Logonserver");

    CreateLoggers();
    utility::GetSafeLogger("console")->info("Running KeycapEmu.Logonserver version {}/{}",
                                            Keycap::Logonserver::Version::GIT_BRANCH,
                                            Keycap::Logonserver::Version::GIT_HASH);

    Keycap::Logonserver::LogonService service;
    service.Start("0.0.0.0", 3724);

    bool running = true;

    Keycap::Logonserver::Cli::RegisterCommands(commands);
    RegisterDefaultCommands(running);

    RunCommandLine(consoleRole, running);

    spdlog::drop_all();
    std::cout << "Hello, World!";
}
