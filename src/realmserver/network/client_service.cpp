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

#include "client_service.hpp"
#include "client_connection.hpp"

namespace keycap::realmserver
{
    bool client_service::on_new_connection(SharedHandler handler)
    {
        return true;
    }

    client_service::SharedHandler client_service::make_handler()
    {
        return std::make_shared<client_connection>(*this, locator_);
    }
}