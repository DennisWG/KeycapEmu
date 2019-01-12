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

#include "network/client_connection.hpp"
#include "network/client_service.hpp"

#include <generated/shared_protocol.hpp>

#include <cli/helpers.hpp>
#include <database/database.hpp>
#include <logging/utility.hpp>
#include <network/services.hpp>
#include <rbac/rbac.hpp>
#include <version.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/network/service_locator.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/utility.hpp>

#include <spdlog/spdlog.h>

#include <botan/bigint.h>
#include <cryptography/packet_scrambler.hpp>
#include <keycap/root/network/srp6/utility.hpp>

namespace logging = keycap::shared::logging;
namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;

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
    } logon_service;

    struct
    {
        uint32 id;
    } realm;
};

config parse_config(std::string config_file)
{
    namespace conf = keycap::root::configuration;
    conf::config_file cfg_file{config_file};

    config cfg;
    cfg.logging = logging::from_config_file(cfg_file);

    cfg.network.bind_ip = cfg_file.get_or_default<std::string>("Network", "BindIp", "127.0.0.1");
    cfg.network.port = cfg_file.get_or_default<int16_t>("Network", "Port", 3724);
    cfg.network.threads = cfg_file.get_or_default<int>("Network", "Threads", 1);

    cfg.account_service.host = cfg_file.get_or_default<std::string>("AccountService", "Host", "127.0.0.1");
    cfg.account_service.port = cfg_file.get_or_default<int16_t>("AccountService", "Port", 6660);

    cfg.logon_service.host = cfg_file.get_or_default<std::string>("LogonService", "Host", "127.0.0.1");
    cfg.logon_service.port = cfg_file.get_or_default<int16_t>("LogonService", "Port", 6662);

    cfg.realm.id = cfg_file.get_or_default<uint32>("Realm", "Id", 1);

    return cfg;
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

void get_realm_info(keycap::root::network::service_locator& locator, config& config)
{
    keycap::protocol::request_realm_data packet;
    packet.realm_id = config.realm.id;

    locator.send_registered(
        shared_net::account_service_type, packet.encode(),
        [&locator, &config](net::service_type sender, net::memory_stream data) {
            if (data.peek<keycap::protocol::shared_command>() != keycap::protocol::shared_command::reply_realm_data)
                return false;

            auto console = keycap::root::utility::get_safe_logger("console");

            console->info("{} located! Attempting to locate {}...", shared_net::account_service.to_string(),
                          shared_net::logon_service.to_string());

            auto packet = keycap::protocol::reply_realm_data::decode(data);

            locator.locate(
                shared_net::logon_realm_service_type, config.logon_service.host, config.logon_service.port,
                [&config, info = packet.info ](auto& locator, auto type) {
                    keycap::protocol::realm_hello packet;
                    packet.info = info;

                    auto console = keycap::root::utility::get_safe_logger("console");
                    console->info("{} located! Sending hello to logon server.", shared_net::logon_service.to_string());
                    locator.send_to(shared_net::logon_realm_service_type, packet.encode());

                    auto exploded_ip = keycap::root::utility::explode(info.ip, ':');
                    auto& ip = exploded_ip[0];
                    auto port = std::stoi(exploded_ip[1]);

                    console->info("Listening to {} on port {} with {} thread(s).", ip, port, config.network.threads);

                    static keycap::realmserver::client_service service{locator, config.network.threads};
                    if (!service.running())
                        service.start(ip, port);
                });

            return true;
        });
}

int main()
{
    namespace utility = keycap::root::utility;

    utility::set_console_title("Realmserver");

    auto config = parse_config("realm.json");
    logging::create_loggers(config.logging);
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Realmserver version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);

    bool running = true;

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

    console->info("Attempting to locate {}...", shared_net::account_service.to_string());
    keycap::root::network::service_locator locator;
    locator.locate(shared_net::account_service_type, config.account_service.host, config.account_service.port,
                   [&config](auto& locator, auto type) { get_realm_info(locator, config); });

    keycap::shared::cli::run_command_line(
        keycap::shared::rbac::role{0, "Console", keycap::shared::rbac::get_all_permissions()}, running);
}