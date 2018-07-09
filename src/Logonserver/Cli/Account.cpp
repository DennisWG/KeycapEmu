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

#include <Cli/Command.hpp>

#include <Database/DAOs/User.hpp>

#include <Database/Database.hpp>
#include <Permissions.hpp>
#include <Rbac/Role.hpp>
#include <User.hpp>

#include <keycap/root/network/srp6/server.hpp>
#include <keycap/root/network/srp6/utility.hpp>
#include <keycap/root/utility/meta.hpp>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/sha160.h>

#include <spdlog/fmt/fmt.h>

#include <iostream>

namespace cli = keycap::shared::cli;
namespace db = keycap::shared::database;
namespace net = keycap::root::network;
namespace rbac = keycap::shared::rbac;

extern keycap::shared::database::database& GetLoginDatabase();

namespace Keycap::Logonserver::Cli
{
    void CreateCommandCallback(std::string const& username, std::string const& email, std::string const& v,
                               std::string const& salt)
    {
        auto dao = db::dal::get_user_dao(GetLoginDatabase());
        dao->create(db::user{0, username, email, v, salt});
    }

    bool CreateCommand(std::vector<std::string> const& args, rbac::role const& role)
    {
        constexpr auto requiredArguments = 3;
        if (args.empty() || args.size() < requiredArguments)
            return false;

        auto username = args[0];
        auto password = args[1];
        auto email = args[2];

        constexpr auto compliance = net::srp6::compliance::Wow;
        auto parameter = net::srp6::get_parameters(net::srp6::group_parameters::_256);

        auto rndSalt = Botan::AutoSeeded_RNG().random_vec(32);
        Botan::BigInt salt = Botan::BigInt::decode({rndSalt});
        auto v = Botan::BigInt::encode(net::srp6::generate_verifier(username, password, parameter, salt, compliance));

        auto hexV = keycap::root::utility::to_hex_string(v.begin(), v.end());
        auto hexSalt = keycap::root::utility::to_hex_string(rndSalt.begin(), rndSalt.end());

        auto dao = db::dal::get_user_dao(GetLoginDatabase());
        dao->user(username, [=](std::optional<keycap::shared::database::user> user) {
            auto dao = db::dal::get_user_dao(GetLoginDatabase());
            if (!user)
                dao->create(db::user{0, username, email, hexV, hexSalt});
        });

        return true;
    }

    cli::command RegisterAccount()
    {
        using keycap::shared::permission;
        using namespace std::string_literals;

        std::vector<cli::command> commands = {
            cli::command{"create", permission::CommandAccountCreate, CreateCommand,
                         "Creates a new account. Arguments: username, password, email"s},
        };

        return cli::command{"account"s, permission::CommandAccount, nullptr, "Account specific commands"s, commands};
    }
}
