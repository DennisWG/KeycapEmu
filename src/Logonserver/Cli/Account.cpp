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

#include <Keycap/Root/Network/Srp6/Server.hpp>
#include <Keycap/Root/Network/Srp6/Utility.hpp>
#include <Keycap/Root/Utility/Meta.hpp>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/sha160.h>

#include <spdlog/fmt/fmt.h>

#include <iostream>

namespace cli = Keycap::Shared::Cli;
namespace db = Keycap::Shared::Database;
namespace net = Keycap::Root::Network;
namespace rbac = Keycap::Shared::Rbac;

extern Keycap::Shared::Database::Database& GetLoginDatabase();

namespace Keycap::Logonserver::Cli
{
    void CreateCommandCallback(std::string const& username, std::string const& email, std::string const& v,
                               std::string const& salt)
    {
        auto dao = db::Dal::user_dao(GetLoginDatabase());
        dao->Create(db::User{0, username, email, v, salt});
    }

    bool CreateCommand(std::vector<std::string> const& args, rbac::Role const& role)
    {
        constexpr auto requiredArguments = 3;
        if (args.empty() || args.size() < requiredArguments)
            return false;

        auto username = args[0];
        auto password = args[1];
        auto email = args[2];

        constexpr auto compliance = net::Srp6::Compliance::Wow;
        auto parameter = net::Srp6::GetParameters(net::Srp6::GroupParameters::_256);

        auto rndSalt = Botan::AutoSeeded_RNG().random_vec(32);
        Botan::BigInt salt = Botan::BigInt::decode({rndSalt});
        auto v = Botan::BigInt::encode(net::Srp6::GenerateVerifier(username, password, parameter, salt, compliance));

        auto hexV = Keycap::Root::Utility::ToHexString(v.begin(), v.end());
        auto hexSalt = Keycap::Root::Utility::ToHexString(rndSalt.begin(), rndSalt.end());

        auto dao = db::Dal::user_dao(GetLoginDatabase());
        dao->User(username, [=](std::optional<Shared::Database::User> user) {
            auto dao = db::Dal::user_dao(GetLoginDatabase());
            if (!user)
                dao->Create(db::User{0, username, email, hexV, hexSalt});
        });

        return true;
    }

    cli::Command RegisterAccount()
    {
        using Keycap::Shared::Permission;
        using namespace std::string_literals;

        std::vector<cli::Command> commands = {
            cli::Command{"create", Permission::CommandAccountCreate, CreateCommand,
                         "Creates a new account. Arguments: username, password, email"s},
        };

        return cli::Command{"account"s, Permission::CommandAccount, nullptr, "Account specific commands"s, commands};
    }
}
