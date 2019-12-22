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

#include "client_connection.hpp"

#include <keycap/root/utility/scope_exit.hpp>

#include <spdlog/spdlog.h>

std::ostream& operator<<(std::ostream& os, std::vector<uint8_t> const& vec)
{
    os << std::hex;

    SCOPE_EXIT(sc, [&os]() { os << std::dec; });

    for (auto&& element : vec)
        os << static_cast<int>(element);

    return os;
}

namespace net = keycap::root::network;

namespace keycap::logonserver
{
    client_connection::client_connection(net::service_base& service, keycap::root::network::service_locator& locator,
                                         realm_manager& realm_manager)
      : base_connection{service}
      , locator_{locator}
      , realm_manager_{realm_manager}
    {
        router_.configure_inbound(this);
    }

    bool client_connection::on_data(net::data_router const& router, net::service_type service,
                                    std::vector<uint8_t> const& data)
    {
        input_stream_.put(gsl::make_span(data));
        // clang-format off
        auto result = std::visit([&](auto& state) -> shared::network::state_result
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            auto cmd = input_stream_.peek<protocol::command>();
            logger->debug("[client_connection] Received {} in state: {}", cmd.to_string(), state.name);

            try
            {
                return state.on_data(router, input_stream_);
            }
            catch (std::exception const& e)
            {
                logger->error(e.what());
                return shared::network::state_result::abort;
            }
            catch (...)
            {
                return shared::network::state_result::abort;
            }

            return shared::network::state_result::abort;
        }, state_);
        // clang-format on

        if (input_stream_.size() > 0 && result != shared::network::state_result::incomplete_data)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->error("[client_connection] packet was under-read! (Left over bytes: {})", input_stream_.size());
            return false;
        }

        return result != shared::network::state_result::abort;
    }

    bool client_connection::on_link(net::data_router const& router, net::service_type service, net::link_status status)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");

        if (status == net::link_status::Up)
        {
            logger->debug("[client_connection] New connection");
            state_ = just_connected{{std::static_pointer_cast<client_connection>(shared_from_this())}};
        }
        else
        {
            logger->debug("[client_connection] Connection closed");
            state_ = disconnected{{std::static_pointer_cast<client_connection>(shared_from_this())}};
        }

        return true;
    }

    void client_connection::send_error(protocol::grunt_result result)
    {
        protocol::server_logon_error error;
        error.result = result;
        send(error.encode());
    }
}
