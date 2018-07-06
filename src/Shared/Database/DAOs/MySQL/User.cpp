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
#include "../../PreparedStatement.hpp"

namespace Keycap::Shared::Database::Dal
{
    class MySqlUserDao final : public UserDao
    {
      public:
        MySqlUserDao(Database& database)
          : database_{database}
        {
        }

        void User(std::string const& username, UserCallback callback) const override
        {
            static auto statement = database_.PrepareStatement("SELECT * FROM user WHERE accountName = ?");
            statement.AddParameter(username);

            auto whenDone = [callback, foo = int(&callback)](std::unique_ptr<sql::ResultSet> result)
            {
                if (!result || !result->next())
                    return callback(std::nullopt);

                Shared::Database::User user{result->getUInt("id"), result->getString("accountName").c_str(),
                                            result->getString("email").c_str(), result->getString("v").c_str(),
                                            result->getString("s").c_str()};
                callback(user);
            };

            statement.QueryAsync(whenDone);
        }

        void Create(Shared::Database::User const& user) const override
        {
            static auto statement
                = database_.PrepareStatement("INSERT INTO user(accountName, email, v, s) VALUES (?, ?, ?, ?)");

            statement.AddParameter(user.accountName);
            statement.AddParameter(user.email);
            statement.AddParameter(user.v);
            statement.AddParameter(user.s);

            statement.ExecuteAsync();
        }

      private:
        Database& database_;
    };

    std::unique_ptr<UserDao> user_dao(Database& database)
    {
        return std::make_unique<MySqlUserDao>(database);
    }
}
