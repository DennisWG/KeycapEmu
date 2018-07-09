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

#include "Connection.hpp"

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;

namespace Keycap::Accountserver
{
    Connection::Connection(net::service_base& service)
      : BaseConnection{service}
    {
        router_.configure_inbound(this);
    }

    bool Connection::on_data(net::service_base& service, std::vector<uint8_t> const& data)
    {
        inputStream_.put(gsl::make_span(data));
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.OnData(*this, service, inputStream_) != Connection::StateResult::Abort;
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

    bool Connection::on_link(net::service_base& service, net::link_status status)
    {
        if (status == net::link_status::Up)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("New connection");
            state_ = JustConnected{};
        }
        else
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Connection closed");
            state_ = Disconnected{};
        }

        return true;
    }

    Connection::StateResult Connection::Disconnected::OnData(Connection& connection, net::service_base& service,
                                                             net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("defuq???");
        return Connection::StateResult::Abort;
    }

    Connection::StateResult Connection::JustConnected::OnData(Connection& connection, net::service_base& service,
                                                              net::memory_stream& stream)
    {
        return Connection::StateResult::Ok;
    }

    Connection::StateResult Connection::Challanged::OnData(Connection& connection, net::service_base& service,
                                                           net::memory_stream& stream)
    {
        return Connection::StateResult::Ok;
    }

    Connection::StateResult Connection::Authenticated::OnData(Connection& connection, net::service_base& service,
                                                              net::memory_stream& stream)
    {
        return Connection::StateResult::Ok;
    }
}