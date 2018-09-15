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

#include "./User.hpp"

#include "../../Database.hpp"
#include "../../prepared_statement.hpp"

namespace keycap::shared::database::dal
{
    class mysql_user_dao final : public user_dao
    {
      public:
        mysql_user_dao(database& database)
          : database_{database}
        {
        }

        void user(std::string const& username, user_callback callback) const override
        {
            static auto statement = database_.prepare_statement("SELECT * FROM user WHERE account_name = ?");
            statement.add_parameter(username);

            auto whenDone = [callback](std::unique_ptr<sql::ResultSet> result) {
                if (!result || !result->next())
                    return callback(std::nullopt);

                shared::database::user user{result->getUInt("id"),
                                            result->getString("account_name").c_str(),
                                            result->getString("email").c_str(),
                                            static_cast<uint8>(result->getUInt("security_options")),
                                            result->getUInt("flags"),
                                            result->getString("v").c_str(),
                                            result->getString("s").c_str()};
                callback(user);
            };

            statement.query_async(whenDone);
        }

        void create(shared::database::user const& user) const override
        {
            static auto statement
                = database_.prepare_statement("INSERT INTO user(account_name, email, v, s) VALUES (?, ?, ?, ?)");

            statement.add_parameter(user.account_name);
            statement.add_parameter(user.email);
            statement.add_parameter(user.v);
            statement.add_parameter(user.s);

            statement.execute_async();
        }

      private:
        database& database_;
    };

    std::unique_ptr<user_dao> get_user_dao(database& database)
    {
        return std::make_unique<mysql_user_dao>(database);
    }
}
