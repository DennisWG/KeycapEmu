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

#include "character_id_provider.hpp"

namespace keycap::accountserver
{
    character_id_provider::character_id_provider(uint32_t start_id)
      : character_id_{start_id}
    {
    }

    uint32_t character_id_provider::character_id() const
    {
        return character_id_;
    }

    uint32_t character_id_provider::generate_next()
    {
        ++character_id_;
        return character_id();
    }
}