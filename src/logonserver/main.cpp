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
#include "network/client_connection.hpp"
#include "network/realm_service.hpp"
#include "realm_manager.hpp"
#include "version.hpp"

#include <cli/helpers.hpp>
#include <crash_dump.hpp>
#include <logging/utility.hpp>
#include <network/services.hpp>
#include <rbac/rbac.hpp>

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
    } account_service;

    struct
    {
        std::string host;
        int16_t port;
    } realm_service;
};

keycap::shared::cli::command_map commands;

void register_command(keycap::shared::cli::command const& command)
{
    commands[command.name] = command;
}

void register_default_commands(bool& running)
{
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
}

auto& get_command_map()
{
    return commands;
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
    conf.logging = logging::from_config_file(cfg_file);

    conf.network.bind_ip = cfg_file.get_or_default<std::string>("Network", "BindIp", "127.0.0.1");
    conf.network.port = cfg_file.get_or_default<int16_t>("Network", "Port", 3724);
    conf.network.threads = cfg_file.get_or_default<int>("Network", "Threads", 1);

    conf.account_service.host = cfg_file.get_or_default<std::string>("AccountService", "Host", "127.0.0.1");
    conf.account_service.port = cfg_file.get_or_default<int16_t>("AccountService", "Port", 6660);

    conf.realm_service.host = cfg_file.get_or_default<std::string>("RealmService", "Host", "127.0.0.1");
    conf.realm_service.port = cfg_file.get_or_default<int16_t>("RealmService", "Port", 6662);

    return conf;
}

int main()
{
    namespace utility = keycap::root::utility;
    namespace net = keycap::root::network;
    namespace shared_net = keycap::shared::network;

    keycap::shared::set_unhandled_exception_handler();

    auto console_role = keycap::shared::rbac::role{0, "Console", keycap::shared::rbac::get_all_permissions()};

    utility::set_console_title("Logonserver");

    auto config = parse_config("logon.json");

    logging::create_loggers(config.logging);
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Logonserver version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);

    bool running = true;
    keycap::logonserver::cli::register_commands(commands);
    register_default_commands(running);

    keycap::logonserver::realm_manager realm_manager;
    keycap::logonserver::realm_service realm_service{1, realm_manager};
    realm_service.start(config.realm_service.host, config.realm_service.port);

    net::service_locator service_locator;
    service_locator.locate(shared_net::account_service_type, config.account_service.host, config.account_service.port);

    keycap::logonserver::logon_service service{config.network.threads, service_locator, realm_manager};
    service.start(config.network.bind_ip, config.network.port);

    keycap::shared::cli::run_command_line(console_role, running);

    std::cout << "Hello, World!";
}
