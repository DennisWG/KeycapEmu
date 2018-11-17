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

#include "client_connection.hpp"
#include "client_service.hpp"

#include <cli/helpers.hpp>
#include <logging/utility.hpp>
#include <network/services.hpp>
#include <rbac/rbac.hpp>

#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/network/service_locator.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/utility.hpp>

#include <database/database.hpp>
#include <version.hpp>

#include <spdlog/spdlog.h>

#include <botan/bigint.h>
#include <cryptography/packet_scrambler.hpp>
#include <keycap/root/network/srp6/utility.hpp>

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

    console->info("Listening to {} on port {} with {} thread(s).", config.network.bind_ip, config.network.port,
                  config.network.threads);

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

    Botan::BigInt s{"0x98C41BA178C741A8D86881615BE4B09552175E80D65CB88D59589EF0B14D54C3"};

    auto dump = [](std::vector<uint8> const& v) {
        for (auto&& n : v)
            printf("%d ", n);
        printf("\n");
    };

    // auto sv = Botan::BigInt::encode(s);
    auto sv = keycap::root::network::srp6::encode_flip(s);
    keycap::shared::cryptography::packet_scrambler scrambler{sv};

    std::vector<uint8> data{1, 2, 3, 4, 5, 6, 7};

    scrambler.encrypt(data.data(), 4);
    dump(data);

    keycap::root::network::service_locator locator;
    locator.locate(keycap::shared::network::account_service, config.account_service.host, config.account_service.port);

    keycap::realmserver::client_service service{locator, config.network.threads};
    service.start(config.network.bind_ip, config.network.port);

    keycap::shared::cli::run_command_line(
        keycap::shared::rbac::role{0, "Console", keycap::shared::rbac::get_all_permissions()}, running);
}