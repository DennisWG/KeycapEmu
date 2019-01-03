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

#include "./realm.hpp"

#include "../../Database.hpp"
#include "../../prepared_statement.hpp"

namespace keycap::shared::database::dal
{
    class mysql_realm_dao final : public realm_dao
    {
      public:
        mysql_realm_dao(database& database)
          : database_{database}
        {
        }

        void realm(uint8 id, realm_callback callback) const override
        {
            static auto statement = database_.prepare_statement("SELECT * FROM realm WHERE id = ?");
            statement.add_parameter(id);

            auto whenDone = [callback](std::unique_ptr<sql::ResultSet> result) {
                if (!result || !result->next())
                    return callback(std::nullopt);

                shared::database::realm realm{
                    static_cast<uint8>(result->getUInt("id")),
                    static_cast<uint8>(result->getUInt("type")),
                    static_cast<uint8>(result->getUInt("locked")),
                    static_cast<uint8>(result->getUInt("flags")),
                    result->getString("name").c_str(),
                    result->getString("host").c_str(),
                    static_cast<uint16>(result->getUInt("port")),
                    {}, // optional<build> allwed_build
                    static_cast<float>(result->getDouble("population")),
                    static_cast<uint8>(result->getUInt("category")),
                };
                callback(realm);
            };

            statement.query_async(whenDone);
        }

      private:
        database& database_;
    };

    std::unique_ptr<realm_dao> get_realm_dao(database& database)
    {
        return std::make_unique<mysql_realm_dao>(database);
    }
}
