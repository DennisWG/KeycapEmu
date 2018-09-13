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
        auto logger = keycap::root::utility::get_safe_logger("connections");
        if (stream.peek<uint8>() != shared_net::protocol::request_account_data)
        {
            logger->error("User send invalid packet! {0}:{1}", __FUNCTION__, __LINE__);
            return connection::state_result::Abort;
        }

        auto request = shared_net::request_account_data::decode(stream);

        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->user(request.account_name, [&, sender](std::optional<shared::database::user> user) {
            shared_net::reply_account_data reply;
            if (user)
            {
                reply.verifier = user->v;
                reply.salt = user->s;
            }

            net::memory_stream out_stream;
            reply.encode(out_stream);
            connection.send_answer(sender, out_stream);
        });

        return connection::state_result::Ok;
    }
}