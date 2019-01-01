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

#include <generated/realm.hpp>

#include <functional>
#include <optional>

namespace keycap::shared::database::dal
{
    class realm_dao
    {
      public:
        virtual ~realm_dao()
        {
        }

        using realm_callback = std::function<void(std::optional<shared::database::realm>)>;
        
        // Retreives the realm with the given id from the database and then calls the given callback
        virtual void realm(uint8 id, realm_callback callback) const = 0;
    };
}