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

#include "am_service.hpp"

#include <cli/helpers.hpp>
#include <database/database.hpp>
#include <logging/utility.hpp>
#include <rbac/rbac.hpp>
#include <version.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/utility.hpp>

#include <functional>

namespace logging = keycap::shared::logging;

struct config
{
    logging::logging_entry logging;

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

config parse_config(std::string configFile)
{
    keycap::root::configuration::config_file cfg_file{configFile};

    config conf;
    conf.logging = logging::from_config_file(cfg_file);

    conf.database.host = cfg_file.get_or_default<std::string>("Database", "Host", "127.0.0.1");
    conf.database.port = cfg_file.get_or_default<int16_t>("Database", "Port", 3306);
    conf.database.user = cfg_file.get_or_default<std::string>("Database", "User", "root");
    conf.database.password = cfg_file.get_or_default<std::string>("Database", "Password", "");
    conf.database.schema = cfg_file.get_or_default<std::string>("Database", "Schema", "");
    conf.database.threads = cfg_file.get_or_default<int>("Database", "Threads", 1);

    return conf;
}

boost::asio::io_service& get_db_service()
{
    static boost::asio::io_service db_service;
    return db_service;
}

keycap::shared::database::database& get_am_database()
{
    static keycap::shared::database::database database{get_db_service()};
    return database;
}

void init_databases(std::vector<std::thread>& thread_pool, config const& config)
{
    get_am_database().connect(config.database.host, config.database.port, config.database.user,
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

keycap::shared::cli::command_map commands;

auto& get_command_map()
{
    return commands;
}

void register_command(keycap::shared::cli::command const& command)
{
    commands[command.name] = command;
}

int main()
{
    namespace utility = keycap::root::utility;

    utility::set_console_title("AccountMessaging");

    auto config = parse_config("account_messaging.json");
    logging::create_loggers(config.logging);
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.AccountMessaging version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);

    boost::asio::io_service::work db_work{get_db_service()};
    std::vector<std::thread> db_thread_pool;
    init_databases(db_thread_pool, config);
    SCOPE_EXIT(sc2, [&] { kill_databases(db_thread_pool); });

    keycap::account_messaging::am_service service;
    bool running = true;

    using namespace std::string_literals;
    using keycap::shared::cli::command;
    using keycap::shared::permission;
    namespace rbac = keycap::shared::rbac;
    register_command(command{"shutdown"s, permission::CommandShutdown,
                             [&](std::vector<std::string> const& args, rbac::role const& role) {
                                 service.stop();
                                 running = false;
                                 return true;
                             },
                             "Shuts down the Server"s});

    service.run();

    keycap::shared::cli::run_command_line(
        keycap::shared::rbac::role{0, "console", keycap::shared::rbac::get_all_permissions()}, running);
}