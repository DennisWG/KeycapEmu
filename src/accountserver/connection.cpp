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

#include <protocol.hpp>

#include <database/daos/user.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;

extern keycap::shared::database::database& get_login_database();

namespace keycap::accountserver
{
    connection::connection(net::service_base& service)
      : net::service_connection{service}
    {
        router_.configure_inbound(this);
    }

    bool connection::on_data(net::data_router const& router, uint64 sender, net::memory_stream& stream)
    {
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Received data in state: {}", state.name);

            try
            {
                return state.on_data(*this, sender, /*input_stream_*/ stream) != connection::state_result::Abort;
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

    bool connection::on_link(net::data_router const& router, net::link_status status)
    {
        if (status == net::link_status::Up)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("New connection");
            state_ = connected{};
        }
        else
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("Connection closed");
            state_ = disconnected{};
        }

        return true;
    }

    connection::state_result connection::disconnected::on_data(connection& connection, uint64 sender,
                                                               net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("defuq???");
        return connection::state_result::Abort;
    }

    connection::state_result connection::connected::on_data(connection& connection, uint64 sender,
                                                            net::memory_stream& stream)
    {
        auto protocol = stream.peek<shared_net::protocol>();
        auto logger = keycap::root::utility::get_safe_logger("connections");

        std::weak_ptr<accountserver::connection> connection_ptr{
            std::static_pointer_cast<accountserver::connection>(connection.shared_from_this())};

        logger->debug("Received {} from {}", protocol.to_string(), sender);

        switch (protocol)
        {
            default:
            {
                logger->error("Received unkown protocol {}", protocol);
                return state_result::Abort;
            }
            case shared_net::protocol::request_account_data:
            {
                auto packet = shared_net::request_account_data::decode(stream);
                return on_account_data_request(connection_ptr, sender, packet);
            }
            case shared_net::protocol::update_session_key:
            {
                auto packet = shared_net::update_session_key::decode(stream);
                return on_update_session_key(connection_ptr, sender, packet);
            }
        }
    }

    connection::state_result
    connection::connected::on_account_data_request(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                   uint64 sender, shared::network::request_account_data& packet)
    {
        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->user(
            packet.account_name, [ sender, connection = connection_ptr ](std::optional<shared::database::user> user) {
                if (connection.expired())
                    return;

                shared_net::reply_account_data reply;

                reply.has_data = user.has_value();
                if (user)
                    reply.data = shared_net::account_data{user->verifier, user->salt, user->security_options, user->flags};

                connection.lock()->send_answer(sender, reply.encode());
            });

        return connection::state_result::Ok;
    }

    connection::state_result
    connection::connected::on_update_session_key(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                 uint64 sender, shared::network::update_session_key& packet)
    {
        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->update_session_key(packet.account_name, packet.session_key);

        return connection::state_result::Ok;
    }
}