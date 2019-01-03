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

#include <cli/command.hpp>

#include <database/daos/user.hpp>

#include <database/database.hpp>
#include <generated/permissions.hpp>
#include <rbac/role.hpp>
#include <generated/user.hpp>

#include <keycap/root/network/srp6/server.hpp>
#include <keycap/root/network/srp6/utility.hpp>
#include <keycap/root/utility/meta.hpp>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/numthry.h>
#include <botan/sha160.h>

#include <spdlog/fmt/fmt.h>

#include <iostream>

namespace db = keycap::shared::database;
namespace net = keycap::root::network;
namespace rbac = keycap::shared::rbac;

extern keycap::shared::database::database& get_login_database();

namespace keycap::accountserver::cli
{
    bool create_command(std::vector<std::string> const& args, rbac::role const& role)
    {
        constexpr auto requiredArguments = 3;
        if (args.empty() || args.size() < requiredArguments)
            return false;

        auto username = args[0];
        auto password = args[1];
        auto email = args[2];

        constexpr auto compliance = net::srp6::compliance::Wow;
        auto parameter = net::srp6::get_parameters(net::srp6::group_parameters::_256);

        auto rnd_salt = Botan::AutoSeeded_RNG().random_vec(32);
        Botan::BigInt salt = Botan::BigInt::decode({rnd_salt});
        auto v = Botan::BigInt::encode(net::srp6::generate_verifier(username, password, parameter, salt, compliance));

        auto hex_v = keycap::root::utility::to_hex_string(v.begin(), v.end(), true);
        auto hex_salt = keycap::root::utility::to_hex_string(rnd_salt.begin(), rnd_salt.end(), true);

        auto dao = db::dal::get_user_dao(get_login_database());
        dao->user(username, [=](std::optional<keycap::shared::database::user> user) {
            auto dao = db::dal::get_user_dao(get_login_database());
            if (!user)
                dao->create(db::user{0, username, email, 0, 0, hex_v, hex_salt});
        });

        return true;
    }

    keycap::shared::cli::command register_account()
    {
        using keycap::shared::permission;
        using namespace std::string_literals;

        std::vector<keycap::shared::cli::command> commands = {
            keycap::shared::cli::command{"create", permission::CommandAccountCreate, create_command,
                                         "Creates a new account. Arguments: username, password, email"s},
        };

        return keycap::shared::cli::command{"account"s, permission::CommandAccount, nullptr,
                                            "Account specific commands"s, commands};
    }
}
