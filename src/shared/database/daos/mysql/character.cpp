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

        virtual uint32 last_id() const override
        {
            static auto statement = database_.prepare_statement("SELECT MAX(`id`) "
                                                                "FROM `character` ");

            auto result = statement.query();
            if (!result || !result->next())
                return 0;

            return result->getUInt("MAX(`id`)");
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

        virtual void create_character(uint8 realm, uint32 character, uint32 user,
                                      keycap::protocol::char_data const& data,
                                      create_character_callback callback) const override
        {
            static auto statement = database_.prepare_statement("SELECT c.*, r. * "
                                                                "FROM realm_character r "
                                                                "INNER JOIN `character` c ON r.`character` = c.id "
                                                                "WHERE (realm = ? AND name = ?) OR id = ?;");

            static auto create_character = database_.prepare_statement(
                "INSERT INTO `character`(id, name, race, player_class, gender, skin, face, hair_style, hair_color, "
                "facial_hair, level, zone, map, x, y, z, guild, flags, first_login, active_pet) VALUES "
                "( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");

            static auto create_realm_character = database_.prepare_statement("INSERT INTO realm_character VALUES "
                                                                             "( ?, ?, ? );");

            statement.add_parameter(realm);
            statement.add_parameter(data.name);
            statement.add_parameter(character);

            auto whenDone = [callback, data, this, realm, character, user](std::unique_ptr<sql::ResultSet> result) {
                if (!result || result->next())
                    return callback(keycap::protocol::char_create_result::name_unavailable);

                create_character.add_parameter(character);
                create_character.add_parameter(data.name);
                create_character.add_parameter(data.race);
                create_character.add_parameter(data.player_class);
                create_character.add_parameter(data.gender);
                create_character.add_parameter(data.skin);
                create_character.add_parameter(data.face);
                create_character.add_parameter(data.hair_style);
                create_character.add_parameter(data.hair_color);
                create_character.add_parameter(data.facial_hair);
                create_character.add_parameter(data.level);
                create_character.add_parameter(data.zone);
                create_character.add_parameter(data.map);
                create_character.add_parameter(data.x);
                create_character.add_parameter(data.y);
                create_character.add_parameter(data.z);
                create_character.add_parameter(data.guild_id);
                create_character.add_parameter(data.flags);
                create_character.add_parameter(data.first_login);
                create_character.add_parameter(data.pet_display_id);

                auto when_created = [this, realm, character, user, callback](bool success) {
                    if (!success)
                        return callback(keycap::protocol::char_create_result::failed);

                    create_realm_character.add_parameter(realm);
                    create_realm_character.add_parameter(character);
                    create_realm_character.add_parameter(user);

                    auto when_realm_char_created = [this, character, callback](bool success) {
                        if (!success)
                        {
                            delete_character(character);
                            return callback(keycap::protocol::char_create_result::error);
                        }

                        callback(keycap::protocol::char_create_result::success);
                    };

                    create_realm_character.execute_async(when_realm_char_created);
                };

                create_character.execute_async(when_created);

                //
            };

            statement.query_async(whenDone);
        }

        virtual void delete_character(uint32 character) const override
        {
            static auto delete_realm_character
                = database_.prepare_statement("DELETE from realm_character WHERE `character` = ?");
            static auto delete_character = database_.prepare_statement("DELETE from `character` WHERE id = ?");

            delete_realm_character.add_parameter(character);
            delete_character.add_parameter(character);

            delete_realm_character.execute_async();
            delete_character.execute_async();
        }

      private:
        database& database_;
    };

    std::unique_ptr<character_dao> get_character_dao(database& database)
    {
        return std::make_unique<mysql_character_dao>(database);
    }
}
