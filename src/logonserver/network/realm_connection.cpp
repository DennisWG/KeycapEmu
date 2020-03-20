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

#include "realm_connection.hpp"

#include <generated/shared_protocol.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;

namespace keycap::logonserver
{
    realm_connection::realm_connection(boost::asio::ip::tcp::socket socket, net::service_base& service,
                                       realm_manager& realm_manager)
      : net::service_connection{std::move(socket), service}
      , realm_manager_{realm_manager}
    {
        router_.configure_inbound(this);
    }

    bool realm_connection::on_data(keycap::root::network::data_router const& router,
                                   keycap::root::network::service_type service, uint64 sender,
                                   keycap::root::network::memory_stream& stream)
    {
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[realm_connection] Received data in state: {}", state.name);

            try
            {
                return state.on_data(*this, sender, stream) != shared::network::state_result::abort;
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

    bool realm_connection::on_link(keycap::root::network::data_router const& router,
                                   keycap::root::network::service_type service,
                                   keycap::root::network::link_status status)
    {
        if (status == net::link_status::Up)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[realm_connection] New connection");
            state_ = connected{};
        }
        else
        {
            realm_manager_.set_offline(realm_id_);

            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[realm_connection] Connection closed");
            state_ = disconnected{};
        }

        return true;
    }

    shared::network::state_result realm_connection::disconnected::on_data(realm_connection& connection, uint64 sender,
                                                                          keycap::root::network::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("[realm_connection] defuq???");
        return shared::network::state_result::abort;
    }

    shared::network::state_result realm_connection::connected::on_data(realm_connection& connection, uint64 sender,
                                                                       keycap::root::network::memory_stream& stream)
    {
        auto command = stream.peek<keycap::protocol::shared_command>();

        if (command != keycap::protocol::shared_command::realm_hello)
            return shared::network::state_result::abort;

        auto packet = keycap::protocol::realm_hello::decode(stream);

        connection.realm_id_ = packet.info.id;

        connection.realm_manager_.insert(packet.info);

        return shared::network::state_result::ok;
    }
}