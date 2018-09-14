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

#include "client_connection.hpp"

namespace keycap::worldserver
{
    client_connection::client_connection(keycap::root::network::service_base& service)
      : connection{service}
    {
    }

    bool client_connection::on_data(keycap::root::network::data_router const& router,
                                    std::vector<uint8_t> const& data)
    {
        return true;
    }

    bool client_connection::on_link(keycap::root::network::data_router const& router,
                                    keycap::root::network::link_status status)
    {
        return true;
    }
}