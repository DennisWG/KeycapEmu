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

#include "cli/registrar.hpp"
#include "client_connection.hpp"
#include "version.hpp"
#include <cli/helpers.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/utility/meta.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/string.hpp>
#include <keycap/root/utility/utility.hpp>

#include <database/daos/user.hpp>
#include <rbac/role.hpp>

#include <spdlog/spdlog.h>

#include <boost/algorithm/string/join.hpp>

#include <iostream>

struct config
{
    struct
    {
        std::string bind_ip;
        int16_t port;
        int threads;
    } network;

    struct
    {
        std::string host;
        int16_t port;
        std::string user;
        std::string password;
        std::string schema;
        int threads;
    } database;
};

keycap::shared::cli::command_map commands;

void register_command(keycap::shared::cli::command const& command)
{
    commands[command.name] = command;
}

void register_default_commands(bool& running)
{
    using namespace std::string_literals;
    using keycap::shared::cli::command;
    using keycap::shared::permission;
    namespace rbac = keycap::shared::rbac;

    register_command(command{"shutdown"s, permission::CommandShutdown,
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

void create_logger(std::string const& name, bool console, bool file, spdlog::level::level_enum level)
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

void create_loggers()
{
    create_logger("console", true, true, spdlog::level::debug);
    create_logger("command", false, true, spdlog::level::debug);
    create_logger("connections", true, true, spdlog::level::debug);
    create_logger("database", true, true, spdlog::level::debug);
}

keycap::shared::rbac::permission_set get_all_permissions()
{
    auto const& perms = keycap::shared::permission::to_vector();
    return keycap::shared::rbac::permission_set{std::begin(perms), std::end(perms)};
}

keycap::shared::rbac::role create_console_role()
{
    return keycap::shared::rbac::role{0, "Console", get_all_permissions()};
}

boost::asio::io_service& get_db_service()
{
    static boost::asio::io_service db_service;
    return db_service;
}

keycap::shared::database::database& get_login_database()
{
    static keycap::shared::database::database login_database{get_db_service()};
    return login_database;
}

void init_databases(std::vector<std::thread>& thread_pool, config const& config)
{
    get_login_database().connect(config.database.host, config.database.port, config.database.user,
                                 config.database.password, config.database.schema);

    auto& service = get_db_service();
    for (int i = 0; i < config.database.threads; ++i)
        thread_pool.emplace_back([&] { service.run(); });
}

void kill_databases(std::vector<std::thread>& thread_pool)
{
    get_db_service().stop();
    for (auto& thread : thread_pool)
    {
        if (thread.joinable())
            thread.join();
    }
}

config parse_config(std::string config_file)
{
    keycap::root::configuration::config_file cfg_file{config_file};

    config conf;
    conf.network.bind_ip = cfg_file.get_or_default<std::string>("Network", "BindIp", "127.0.0.1");
    conf.network.port = cfg_file.get_or_default<int16_t>("Network", "Port", 3724);
    conf.network.threads = cfg_file.get_or_default<int>("Network", "Threads", 1);

    conf.database.host = cfg_file.get_or_default<std::string>("Database", "Host", "127.0.0.1");
    conf.database.port = cfg_file.get_or_default<int16_t>("Database", "Port", 3306);
    conf.database.user = cfg_file.get_or_default<std::string>("Database", "User", "root");
    conf.database.password = cfg_file.get_or_default<std::string>("Database", "Password", "");
    conf.database.schema = cfg_file.get_or_default<std::string>("Database", "Schema", "");
    conf.database.threads = cfg_file.get_or_default<int>("Database", "Threads", 1);

    return conf;
}

int main()
{
    namespace utility = keycap::root::utility;

    auto console_role = create_console_role();

    utility::set_console_title("Logonserver");

    create_loggers();
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Logonserver version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);

    auto config = parse_config("config.json");

    boost::asio::io_service::work db_work{get_db_service()};
    std::vector<std::thread> db_thread_pool;
    init_databases(db_thread_pool, config);
    SCOPE_EXIT(sc2, [&] { kill_databases(db_thread_pool); });

    bool running = true;

    keycap::logonserver::cli::register_commands(commands);
    register_default_commands(running);

    keycap::logonserver::logon_service service{config.network.threads};
    service.start(config.network.bind_ip, config.network.port);

    // keycap::logonserver::

    /////////////////////////
    using namespace std::string_literals;
    using keycap::shared::cli::command;
    using keycap::shared::permission;
    namespace rbac = keycap::shared::rbac;

    register_command(command{"db"s, permission::CommandShutdown,
                             [&](std::vector<std::string> const& args, rbac::role const& role) {
                                 console->info("Connected: {}", get_login_database().is_connected());
                                 return true;
                             },
                             "db"s});

    register_command(command{"query"s, permission::CommandShutdown,
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
                                 auto& db = get_login_database();
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

    keycap::shared::cli::run_command_line(console_role, running);

    std::cout << "Hello, World!";
}
