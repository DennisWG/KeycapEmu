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

#include "account_connection.hpp"
#include "account_service.hpp"
#include "cli/registrar.hpp"
#include "client_connection.hpp"
#include "version.hpp"
#include <cli/helpers.hpp>

#include <network/services.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/network/data_router.hpp>
#include <keycap/root/network/service_locator.hpp>
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
    } account_service;
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

config parse_config(std::string config_file)
{
    keycap::root::configuration::config_file cfg_file{config_file};

    config conf;
    conf.network.bind_ip = cfg_file.get_or_default<std::string>("Network", "BindIp", "127.0.0.1");
    conf.network.port = cfg_file.get_or_default<int16_t>("Network", "Port", 3724);
    conf.network.threads = cfg_file.get_or_default<int>("Network", "Threads", 1);

    conf.account_service.host = cfg_file.get_or_default<std::string>("AccountService", "Host", "127.0.0.1");
    conf.account_service.port = cfg_file.get_or_default<int16_t>("AccountService", "Port", 3306);

    return conf;
}

int main()
{
    namespace utility = keycap::root::utility;
    namespace net = keycap::root::network;
    namespace shared_net = keycap::shared::network;

    auto console_role = create_console_role();

    utility::set_console_title("Logonserver");

    create_loggers();
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Logonserver version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);

    auto config = parse_config("logon.json");

    bool running = true;
    keycap::logonserver::cli::register_commands(commands);
    register_default_commands(running);

    net::service_locator service_locator;
    service_locator.locate(shared_net::account_service, config.account_service.host, config.account_service.port);

    keycap::logonserver::logon_service service{config.network.threads, service_locator};
    service.start(config.network.bind_ip, config.network.port);

    keycap::shared::cli::run_command_line(console_role, running);

    std::cout << "Hello, World!";
}
