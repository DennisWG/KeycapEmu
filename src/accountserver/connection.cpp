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

#include "connection.hpp"

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;

namespace keycap::accountserver
{
    connection::connection(net::service_base& service)
      : base_connection{service}
    {
        router_.configure_inbound(this);
    }

    bool connection::on_data(net::service_base& service, std::vector<uint8_t> const& data)
    {
        input_stream_.put(gsl::make_span(data));
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.on_data(*this, service, input_stream_) != connection::state_result::Abort;
            }
            catch (std::exception const& e)
            {
                logger->error(e.what());
                return false;
            }
            catch (...)
            {
                return false;
            }

            return false;
        }, state_);
        // clang-format on
    }

    bool connection::on_link(net::service_base& service, net::link_status status)
    {
        if (status == net::link_status::Up)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("New connection");
            state_ = just_connected{};
        }
        else
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Connection closed");
            state_ = disconnected{};
        }

        return true;
    }

    connection::state_result connection::disconnected::on_data(connection& connection, net::service_base& service,
                                                               net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("defuq???");
        return connection::state_result::Abort;
    }

    connection::state_result connection::just_connected::on_data(connection& connection, net::service_base& service,
                                                                 net::memory_stream& stream)
    {
        return connection::state_result::Ok;
    }

    connection::state_result connection::challanged::on_data(connection& connection, net::service_base& service,
                                                             net::memory_stream& stream)
    {
        return connection::state_result::Ok;
    }

    connection::state_result connection::authenticated::on_data(connection& connection, net::service_base& service,
                                                                net::memory_stream& stream)
    {
        return connection::state_result::Ok;
    }
}