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

#include "./user.hpp"

#include "../../database.hpp"
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
                                            result->getString("verifier").c_str(),
                                            result->getString("salt").c_str(),
                                            result->getString("session_key").c_str()};
                callback(user);
            };

            statement.query_async(whenDone);
        }

        void create(shared::database::user const& user) const override
        {
            static auto statement
                = database_.prepare_statement("INSERT INTO user(account_name, email, security_options, flags, "
                                              "verifier, salt) VALUES (?, ?, ?, ?, ?, ?)");

            statement.add_parameter(user.account_name);
            statement.add_parameter(user.email);
            statement.add_parameter(user.security_options);
            statement.add_parameter(user.flags);
            statement.add_parameter(user.verifier);
            statement.add_parameter(user.salt);

            statement.execute_async();
        }

        virtual void update_session_key(std::string const& account_name, std::string const& session_key) const override
        {
            static auto statement
                = database_.prepare_statement("UPDATE user SET session_key = ? WHERE account_name = ?");

            statement.add_parameter(session_key);
            statement.add_parameter(account_name);

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
