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

#include "../client_connection.hpp"

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;

namespace keycap::realmserver
{
    client_connection::state_result client_connection::disconnected::on_data(client_connection& connection,
                                                                             net::data_router const& router,
                                                                             net::memory_stream& stream)
    {
        auto logger = root::utility::get_safe_logger("connections");
        logger->error("[client_connection::disconnected] defuq???");
        return std::make_tuple(shared::network::state_result::abort, uint16{0}, protocol::client_command::invalid);
    }
}