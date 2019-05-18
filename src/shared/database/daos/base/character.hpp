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

#include <generated/character.hpp>

#include <functional>
#include <optional>

namespace keycap::shared::database::dal
{
    class character_dao
    {
      public:
        virtual ~character_dao()
        {
        }

        using character_callback = std::function<void(std::vector<shared::database::character>)>;

        // Retreives all characters from the given realm with the given user id from the database and then calls the
        // given callback
        virtual void realm_characters(uint8 realm, uint32 user, character_callback callback) const = 0;
    };
}