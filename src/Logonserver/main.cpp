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
#include "ClientConnection.hpp"
#include "Version.hpp"
#include <Cli/Helpers.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/utility/meta.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/string.hpp>
#include <keycap/root/utility/utility.hpp>

#include <Database/DAOs/User.hpp>
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

keycap::shared::cli::command_map commands;

void RegisterCommand(keycap::shared::cli::command const& command)
{
    commands[command.name] = command;
}

void RegisterDefaultCommands(bool& running)
{
    using namespace std::string_literals;
    using keycap::shared::cli::command;
    using keycap::shared::permission;
    namespace rbac = keycap::shared::rbac;

    RegisterCommand(command{"shutdown"s, permission::CommandShutdown,
                            [&running](std::vector<std::string> const& args, rbac::role const& role) {
                                running = false;
                                return true;
                            },
                            "Shuts down the Server"s});
}

auto& get_command_map()
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

keycap::shared::rbac::permission_set GetAllPermissions()
{
    auto const& perms = keycap::shared::permission::to_vector();
    return keycap::shared::rbac::permission_set{std::begin(perms), std::end(perms)};
}

keycap::shared::rbac::role CreateConsoleRole()
{
    return keycap::shared::rbac::role{0, "Console", GetAllPermissions()};
}

boost::asio::io_service& GetDbService()
{
    static boost::asio::io_service dbService;
    return dbService;
}

keycap::shared::database::database& GetLoginDatabase()
{
    static keycap::shared::database::database loginDatabase{GetDbService()};
    return loginDatabase;
}

void InitDatabases(std::vector<std::thread>& threadPool, Config const& config)
{
    GetLoginDatabase().connect(config.Database.Host, config.Database.Port, config.Database.User,
                               config.Database.Password, config.Database.Schema);

    auto& service = GetDbService();
    for (int i = 0; i < config.Database.Threads; ++i)
        threadPool.emplace_back([&] { service.run(); });
}

void KillDatabases(std::vector<std::thread>& threadPool)
{
    GetDbService().stop();
    for (auto& thread : threadPool)
    {
        if (thread.joinable())
            thread.join();
    }
}

Config ParseConfig(std::string configFile)
{
    keycap::root::configuration::config_file cfgFile{configFile};

    Config conf;
    conf.Network.BindIp = cfgFile.get_or_default<std::string>("Network", "BindIp", "127.0.0.1");
    conf.Network.Port = cfgFile.get_or_default<int16_t>("Network", "Port", 3724);
    conf.Network.Threads = cfgFile.get_or_default<int>("Network", "Threads", 1);

    conf.Database.Host = cfgFile.get_or_default<std::string>("Database", "Host", "127.0.0.1");
    conf.Database.Port = cfgFile.get_or_default<int16_t>("Database", "Port", 3306);
    conf.Database.User = cfgFile.get_or_default<std::string>("Database", "User", "root");
    conf.Database.Password = cfgFile.get_or_default<std::string>("Database", "Password", "");
    conf.Database.Schema = cfgFile.get_or_default<std::string>("Database", "Schema", "");
    conf.Database.Threads = cfgFile.get_or_default<int>("Database", "Threads", 1);

    return conf;
}

int main()
{
    namespace utility = keycap::root::utility;

    auto consoleRole = CreateConsoleRole();

    utility::set_console_title("Logonserver");

    CreateLoggers();
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Logonserver version {}/{}", Keycap::Logonserver::Version::GIT_BRANCH,
                  Keycap::Logonserver::Version::GIT_HASH);

    auto config = ParseConfig("config.json");

    boost::asio::io_service::work dbWork{GetDbService()};
    std::vector<std::thread> dbThreadPool;
    InitDatabases(dbThreadPool, config);
    SCOPE_EXIT(sc2, [&] { KillDatabases(dbThreadPool); });

    bool running = true;

    Keycap::Logonserver::Cli::RegisterCommands(commands);
    RegisterDefaultCommands(running);

    Keycap::Logonserver::LogonService service{config.Network.Threads};
    service.start(config.Network.BindIp, config.Network.Port);

    //Keycap::Logonserver::

    /////////////////////////
    using namespace std::string_literals;
    using keycap::shared::cli::command;
    using keycap::shared::permission;
    namespace rbac = keycap::shared::rbac;

    RegisterCommand(command{"db"s, permission::CommandShutdown,
                            [&](std::vector<std::string> const& args, rbac::role const& role) {
                                console->info("Connected: {}", GetLoginDatabase().is_connected());
                                return true;
                            },
                            "db"s});

    RegisterCommand(command{"query"s, permission::CommandShutdown,
                            [&](std::vector<std::string> const& args, rbac::role const& role) {
                                /*
                                auto statement = db.PrepareStatement("SELECT * FROM user");

                                auto result = statement.Query();

                                while (result->next())
                                {
                                    console->info("{}, {}, {}, {}, {}", result->getInt("id"),
                                                  result->getString("accountName").c_str(),
                                                  result->getString("email").c_str(), result->getString("v").c_str(),
                                                  result->getString("s").c_str());
                                }
                                */

                                using callback = keycap::shared::database::dal::user_dao::user_callback;
                                auto& db = GetLoginDatabase();
                                auto dao = keycap::shared::database::dal::get_user_dao(db);
                                dao->user(args[0], [&](std::optional<keycap::shared::database::user> user) {
                                    if (user)
                                        console->info("{}, {}, {}, {}, {}", user->id, user->accountName, user->email,
                                                      user->v, user->s);
                                    else
                                        console->info("User couldn't be found!");
                                });
                                return true;
                            },
                            "query"s});
    ////////////////////////*/

    keycap::shared::cli::run_command_line(consoleRole, running);

    std::cout << "Hello, World!";
}
