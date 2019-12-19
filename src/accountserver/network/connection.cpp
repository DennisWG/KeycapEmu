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

#include "connection.hpp"

#include <generated/shared_protocol.hpp>

#include <database/daos/character.hpp>
#include <database/daos/realm.hpp>
#include <database/daos/user.hpp>
#include <database/daos/user_telemetry.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;
namespace protocol = keycap::protocol;

extern keycap::shared::database::database& get_login_database();

namespace keycap::accountserver
{
    connection::connection(net::service_base& service)
      : net::service_connection{service}
    {
        router_.configure_inbound(this);
    }

    bool connection::on_data(net::data_router const& router, net::service_type service, uint64 sender,
                             net::memory_stream& stream)
    {
        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[connection] Received data in state: {}", state.name);

            try
            {
                return state.on_data(*this, sender, /*input_stream_*/ stream) != shared::network::state_result::abort;
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

    bool connection::on_link(net::data_router const& router, net::service_type service, net::link_status status)
    {
        if (status == net::link_status::Up)
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[connection] New connection");
            state_ = connected{};
        }
        else
        {
            auto logger = keycap::root::utility::get_safe_logger("connections");
            logger->debug("[connection] Connection closed");
            state_ = disconnected{};
        }

        return true;
    }

    shared::network::state_result connection::disconnected::on_data(connection& connection, uint64 sender,
                                                                    net::memory_stream& stream)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->error("[connection] defuq???");
        return shared::network::state_result::abort;
    }

    shared::network::state_result connection::connected::on_data(connection& connection, uint64 sender,
                                                                 net::memory_stream& stream)
    {
        auto command = stream.peek<protocol::shared_command>();
        auto logger = keycap::root::utility::get_safe_logger("connections");

        std::weak_ptr<accountserver::connection> connection_ptr{
            std::static_pointer_cast<accountserver::connection>(connection.shared_from_this())};

        logger->debug("[connection] Received {} from {}", command.to_string(), sender);

        switch (command)
        {
            default:
            {
                logger->error("[connection] Received unkown command {}", command);
                return shared::network::state_result::abort;
            }
            case protocol::shared_command::request_account_data:
            {
                auto packet = protocol::request_account_data::decode(stream);
                return on_account_data_request(connection_ptr, sender, packet);
            }
            case protocol::shared_command::update_session_key:
            {
                auto packet = protocol::update_session_key::decode(stream);
                return on_update_session_key(connection_ptr, sender, packet);
            }
            case protocol::shared_command::request_session_key:
            {
                auto packet = protocol::request_session_key::decode(stream);
                return on_session_key_request(connection_ptr, sender, packet);
            }
            case protocol::shared_command::request_realm_data:
            {
                auto packet = protocol::request_realm_data::decode(stream);
                return on_realm_data_request(connection_ptr, sender, packet);
            }
            case protocol::shared_command::request_characters:
            {
                auto packet = protocol::request_characters::decode(stream);
                return on_characters_request(connection_ptr, sender, packet);
            }
            case protocol::shared_command::request_account_id_from_name:
            {
                auto packet = protocol::request_account_id_from_name::decode(stream);
                return on_request_account_id_from_name(connection_ptr, sender, packet);
            }
            case protocol::shared_command::login_telemetry:
            {
                auto packet = protocol::login_telemetry::decode(stream);
                return on_login_telemetry(connection_ptr, sender, packet);
            }
        }
    }

    shared::network::state_result
    connection::connected::on_account_data_request(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                   uint64 sender, protocol::request_account_data& packet)
    {
        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->user(packet.account_name, [sender,
                                             connection = connection_ptr](std::optional<shared::database::user> user) {
            if (connection.expired())
                return;

            protocol::reply_account_data reply;

            if (user)
                reply.data = protocol::account_data{user->verifier, user->salt, user->security_options, user->flags};

            connection.lock()->send_answer(sender, reply.encode());
        });

        return shared::network::state_result::ok;
    }

    shared::network::state_result
    connection::connected::on_update_session_key(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                 uint64 sender, protocol::update_session_key& packet)
    {
        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->update_session_key(packet.account_name, packet.session_key);

        return shared::network::state_result::ok;
    }

    shared::network::state_result
    connection::connected::on_session_key_request(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                  uint64 sender, protocol::request_session_key& packet)
    {
        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->user(packet.account_name,
                       [sender, connection = connection_ptr](std::optional<shared::database::user> user) {
                           if (connection.expired() || !user)
                               return;

                           protocol::reply_session_key reply;
                           reply.session_key = user->session_key;

                           connection.lock()->send_answer(sender, reply.encode());
                       });

        return shared::network::state_result::ok;
    }

    shared::network::state_result
    connection::connected::on_realm_data_request(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                 uint64 sender, protocol::request_realm_data& packet)
    {
        auto realm_dao = shared::database::dal::get_realm_dao(get_login_database());

        realm_dao->realm(packet.realm_id,
                         [sender, connection = connection_ptr](std::optional<shared::database::realm> realm) {
                             if (connection.expired() || !realm)
                                 return;

                             protocol::reply_realm_data reply;
                             reply.info = {static_cast<protocol::realm_type>(realm->type),         realm->locked,
                                           static_cast<protocol::realm_flag>(realm->flags),        realm->name,
                                           fmt::format("{}:{}", realm->host, realm->port),         realm->population,
                                           static_cast<protocol::realm_category>(realm->category), realm->id};

                             connection.lock()->send_answer(sender, reply.encode());
                         });

        return shared::network::state_result::ok;
    }

    shared::network::state_result
    connection::connected::on_characters_request(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                 uint64 sender, protocol::request_characters& packet)
    {
        auto char_dao = shared::database::dal::get_character_dao(get_login_database());

        char_dao->realm_characters(
            packet.realm_id, packet.account_id,
            [sender, connection = connection_ptr](std::vector<shared::database::character> characters) {
                if (connection.expired())
                    return;

                protocol::reply_characters reply;

                for (auto& character : characters)
                {
                    reply.characters.emplace_back(protocol::char_data{
                        character.id,         character.name,        character.race,        character.player_class,
                        character.gender,     character.skin,        character.face,        character.hair_style,
                        character.hair_color, character.facial_hair, character.level,       character.zone,
                        character.map,        character.x,           character.y,           character.z,
                        character.guild,      character.flags,       character.first_login, character.active_pet,
                        // character.items,
                    });
                }

                connection.lock()->send_answer(sender, reply.encode());
            });

        return shared::network::state_result::ok;
    }

    shared::network::state_result
    connection::connected::on_request_account_id_from_name(std::weak_ptr<accountserver::connection>& connection_ptr,
                                                           uint64 sender,
                                                           protocol::request_account_id_from_name& packet)
    {
        auto user_dao = shared::database::dal::get_user_dao(get_login_database());
        user_dao->user_id_from_username(packet.account_name,
                                        [sender, connection = connection_ptr](std::optional<int> user_id) {
                                            if (connection.expired() || !user_id)
                                                return;

                                            protocol::reply_account_id reply;
                                            reply.account_id = *user_id;

                                            connection.lock()->send_answer(sender, reply.encode());
                                        });

        return shared::network::state_result::ok;
    }

    shared::network::state_result
    connection::connected::on_login_telemetry(std::weak_ptr<accountserver::connection>& connection_ptr, uint64 sender,
                                              protocol::login_telemetry& packet)
    {
        auto telemetry_dao = shared::database::dal::get_user_telemetry_dao(get_login_database());
        telemetry_dao->add_telemetry_data(packet.account_name, packet.telemetry);

        return shared::network::state_result::ok;
    }
}