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

#include "./user_telemetry.hpp"

#include "../../database.hpp"
#include "../../prepared_statement.hpp"

namespace keycap::shared::database::dal
{
    class mysql_user_telemetry_dao final : public user_telemetry_dao
    {
      public:
        mysql_user_telemetry_dao(database& database)
          : database_{database}
        {
        }

        void add_telemetry_data(std::string const& account_name, std::string const& data) override
        {
            static auto statement
                = database_.prepare_statement("INSERT INTO user_telemetry (date_taken, telemetry, id) VALUES ( ?, ?, "
                                              "(SELECT id FROM user WHERE account_name = ?));");

            statement.add_parameter(time(nullptr));
            statement.add_parameter(data);
            statement.add_parameter(account_name);

            statement.execute_async();
        }

      private:
        database& database_;
    };

    std::unique_ptr<user_telemetry_dao> get_user_telemetry_dao(database& database)
    {
        return std::make_unique<mysql_user_telemetry_dao>(database);
    }
}
