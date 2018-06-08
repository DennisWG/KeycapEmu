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

#include <Keycap/Root/Configuration/ConfigFile.hpp>
#include <Keycap/Root/Utility/ScopeExit.hpp>
#include <Keycap/Root/Utility/String.hpp>
#include <Keycap/Root/Utility/Utility.hpp>

#include <Database/Database.hpp>
#include <Rbac/Role.hpp>

#include <spdlog/spdlog.h>

#include <boost/algorithm/string/join.hpp>

#include <iostream>

struct Config
{
    struct
    {
        std::string BindIp;
        int16_t Port;
        int Threads;
    } Network;

    struct
    {
        std::string Host;
        int16_t Port;
        std::string User;
        std::string Password;
        std::string Schema;
        int Threads;
    } Database;
};

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
    CreateLogger("database", true, true, spdlog::level::debug);
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
        QUICK_SCOPE_EXIT(sc, []() { std::cout << '>'; });

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

boost::asio::io_service& GetDbService()
{
    static boost::asio::io_service dbService;
    return dbService;
}

Keycap::Shared::Database::Database& GetLoginDatabase()
{
    static Keycap::Shared::Database::Database loginDatabase{GetDbService()};
    return loginDatabase;
}

void InitDatabases(std::vector<std::thread>& threadPool, Config const& config)
{
    GetLoginDatabase().Connect(config.Database.Host, config.Database.Port, config.Database.User,
                               config.Database.Password, config.Database.Schema);

    auto& service = GetDbService();
    for (int i = 0; i < config.Database.Threads; ++i)
        threadPool.emplace_back([&] { service.run(); });
}

Config ParseConfig(std::string configFile)
{
    Keycap::Root::Configuration::ConfigFile cfgFile{configFile};

    Config conf;
    conf.Network.BindIp = cfgFile.GetOrDefault<std::string>("Network", "BindIp", "127.0.0.1");
    conf.Network.Port = cfgFile.GetOrDefault<int16_t>("Network", "Port", 3724);
    conf.Network.Threads = cfgFile.GetOrDefault<int>("Network", "Threads", 1);

    conf.Database.Host = cfgFile.GetOrDefault<std::string>("Database", "Host", "127.0.0.1");
    conf.Database.Port = cfgFile.GetOrDefault<int16_t>("Database", "Port", 3306);
    conf.Database.User = cfgFile.GetOrDefault<std::string>("Database", "User", "root");
    conf.Database.Password = cfgFile.GetOrDefault<std::string>("Database", "Password", "");
    conf.Database.Schema = cfgFile.GetOrDefault<std::string>("Database", "Schema", "");
    conf.Database.Threads = cfgFile.GetOrDefault<int>("Database", "Threads", 1);

    return conf;
}

int main()
{
    namespace utility = Keycap::Root::Utility;

    auto consoleRole = CreateConsoleRole();

    utility::SetConsoleTitle("Logonserver");

    CreateLoggers();
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::GetSafeLogger("console");
    console->info("Running KeycapEmu.Logonserver version {}/{}", Keycap::Logonserver::Version::GIT_BRANCH,
                  Keycap::Logonserver::Version::GIT_HASH);

    auto config = ParseConfig("config.json");

    boost::asio::io_service::work dbWork{GetDbService()};
    std::vector<std::thread> dbThreadPool;
    InitDatabases(dbThreadPool, config);

    SCOPE_EXIT(sc2, [&] {
        GetDbService().stop();
        for (auto& thread : dbThreadPool)
        {
            if (thread.joinable())
                thread.join();
        }
    });

    bool running = true;

    Keycap::Logonserver::Cli::RegisterCommands(commands);
    RegisterDefaultCommands(running);

    Keycap::Logonserver::LogonService service{config.Network.Threads};
    service.Start(config.Network.BindIp, config.Network.Port);

    /////////////////////////
    using namespace std::string_literals;
    using Keycap::Shared::Permission;
    using Keycap::Shared::Cli::Command;
    namespace rbac = Keycap::Shared::Rbac;

    RegisterCommand(Command{"db"s, Permission::CommandShutdown,
                            [&](std::vector<std::string> const& args, rbac::Role const& role) {
                                console->info("Connected: {}", GetLoginDatabase().IsConnected());
                                return true;
                            },
                            "db"s});

    RegisterCommand(
        Command{"query"s, Permission::CommandShutdown,
                [&](std::vector<std::string> const& args, rbac::Role const& role) {
                    auto stmt = boost::algorithm::join(args, " ");
                    GetLoginDatabase().Query(stmt, [](boost::system::error_code const& ec, amy::result_set result) {
                        if (ec)
                        {
                            auto logger = Keycap::Root::Utility::GetSafeLogger("database");
                            logger->error("Message: {}, Error Code: {}", ec.message(), ec.value());
                            return;
                        }

                        auto console = utility::GetSafeLogger("console");
                        console->info("Affected: {}, field count: {}, size: {}", result.affected_rows(),
                                      result.field_count(), result.size());
                        console->info("Rows:");
                        for (const auto& row : result)
                        {
                            console->info("{}, {}, {}, {}, {}", row[0].as<amy::sql_bigint>(), row[1].as<std::string>(),
                                          row[2].as<std::string>(), row[3].as<std::string>(), row[4].as<std::string>());
                        }
                    });
                    return true;
                },
                "query"s});
    /////////////////////////

    RunCommandLine(consoleRole, running);

    std::cout << "Hello, World!";
}
