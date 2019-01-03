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

#pragma once

#include <keycap/root/utility/enum.hpp>

namespace keycap::shared::network
{
    keycap_enum(state_result, uint32_t,
                // We've received the packet as intended and are ready to move on to the next state
                ok = 0,
                // There was some kind of error in the received packet and we have to terminate the connection
                abort = 1,
                // We're still wating for more data to arrive from the client
                incomplete_data = 2,
    );
}