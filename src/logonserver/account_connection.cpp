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

#include "account_connection.hpp"

#include <botan/srp6.h>
#include <spdlog/spdlog.h>

namespace net = keycap::root::network;

namespace keycap::logonserver
{
    account_connection::account_connection(keycap::root::network::service_base& service)
      : BaseConnection{service}
    {
        router_.configure_inbound(this);
    }

    bool account_connection::on_data(net::data_router const& router, net::service_type service,
                                     std::vector<uint8_t> const& data)
    {
        input_stream_.put(gsl::make_span(data));
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.on_data(*this, router, input_stream_) != shared::network::state_result::abort;
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

    bool account_connection::on_link(net::data_router const& router, net::service_type service, net::link_status status)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");

        if (status == net::link_status::Up)
        {
            logger->debug("New connection");
            state_ = connected{};
        }
        else
        {
            logger->debug("Connection closed");
            state_ = disconnected{};
        }

        return false;
    }

    shared::network::state_result
    account_connection::disconnected::on_data(account_connection& connection,
                                              keycap::root::network::data_router const& router,
                                              keycap::root::network::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("Received data while disconnected. This is a bug. Do something!");
        return shared::network::state_result::abort;
    }

    shared::network::state_result
    account_connection::connected::on_data(account_connection& connection,
                                           keycap::root::network::data_router const& router,
                                           keycap::root::network::memory_stream& stream)
    {
        return shared::network::state_result::ok;
    }
}