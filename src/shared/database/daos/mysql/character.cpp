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

#include "character.hpp"

#include "../../database.hpp"
#include "../../prepared_statement.hpp"

namespace keycap::shared::database::dal
{
    class mysql_character_dao final : public character_dao
    {
      public:
        mysql_character_dao(database& database)
          : database_{database}
        {
        }

        void realm_characters(uint8 realm, uint32 user, character_callback callback) const override
        {
            static auto statement = database_.prepare_statement("SELECT c.*, i.* " 
                                                                "from realm_character r "
                                                                    "INNER JOIN `character` c ON r.`character` = c.id "
                                                                    "LEFT JOIN character_item i ON i.`character` = c.id "
                                                                "WHERE account = ? AND realm = ?;");
            statement.add_parameter(user);
            statement.add_parameter(realm);

            auto whenDone = [callback](std::unique_ptr<sql::ResultSet> result) {
                if (!result || !result->next())
                    return callback({});

                std::vector<shared::database::character> characters;
                do
                {
                    characters.emplace_back(shared::database::character{
                        static_cast<uint32>(result->getUInt("id")),
                        result->getString("name").c_str(),
                        static_cast<uint8>(result->getUInt("race")),
                        static_cast<uint8>(result->getUInt("player_class")),
                        static_cast<uint8>(result->getUInt("gender")),
                        static_cast<uint8>(result->getUInt("skin")),
                        static_cast<uint8>(result->getUInt("face")),
                        static_cast<uint8>(result->getUInt("hair_style")),
                        static_cast<uint8>(result->getUInt("hair_color")),
                        static_cast<uint8>(result->getUInt("facial_hair")),
                        static_cast<uint8>(result->getUInt("level")),
                        static_cast<uint32>(result->getUInt("zone")),
                        static_cast<uint32>(result->getUInt("map")),
                        static_cast<float>(result->getUInt("x")),
                        static_cast<float>(result->getUInt("y")),
                        static_cast<float>(result->getUInt("z")),
                        static_cast<uint32>(result->getUInt("guild")),
                        static_cast<uint32>(result->getUInt("flags")),
                        static_cast<uint8>(result->getUInt("first_login")),
                        static_cast<uint32>(result->getUInt("active_pet")),
                        // std::vector<character_item> items;
                    });
                } while (result->next());
                callback(characters);
            };

            statement.query_async(whenDone);
        }

      private:
        database& database_;
    };

    std::unique_ptr<character_dao> get_character_dao(database& database)
    {
        return std::make_unique<mysql_character_dao>(database);
    }
}
