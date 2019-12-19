/*
    Copyright 2018-2019 KeycapEmu

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
#include "network/connection.hpp"

#include <cli/helpers.hpp>
#include <crash_dump.hpp>
#include <database/database.hpp>
#include <logging/utility.hpp>
#include <rbac/rbac.hpp>
#include <version.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/utility.hpp>

#include <spdlog/spdlog.h>

namespace logging = keycap::shared::logging;

struct config
{
    logging::logging_entry logging;

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

config parse_config(std::string configFile)
{
    keycap::root::configuration::config_file cfg_file{configFile};

    config conf;
    conf.logging = logging::from_config_file(cfg_file);

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

    keycap::shared::set_unhandled_exception_handler();

    utility::set_console_title("Accountserver");

    auto config = parse_config("account.json");
    logging::create_loggers(config.logging);
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Accountserver version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);

    console->info("Listening to {} on port {} with {} thread(s).", config.network.bind_ip, config.network.port,
                  config.network.threads);

    boost::asio::io_service::work db_work{get_db_service()};
    std::vector<std::thread> db_thread_pool;
    init_databases(db_thread_pool, config);
    SCOPE_EXIT(sc2, [&] { kill_databases(db_thread_pool); });

    bool running = true;
    keycap::accountserver::cli::register_commands(commands);

    using namespace std::string_literals;
    using keycap::shared::permission;
    using keycap::shared::cli::command;
    namespace rbac = keycap::shared::rbac;
    register_command(command{"shutdown"s, permission::CommandShutdown,
                             [&running](std::vector<std::string> const& args, rbac::role const& role) {
                                 running = false;
                                 return true;
                             },
                             "Shuts down the Server"s});

    keycap::accountserver::account_service service{config.network.threads};
    service.start(config.network.bind_ip, config.network.port);

    keycap::shared::cli::run_command_line(
        keycap::shared::rbac::role{0, "console", keycap::shared::rbac::get_all_permissions()}, running);
}