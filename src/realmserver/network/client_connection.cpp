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

#include <generated/authentication.hpp>
#include <generated/realm_protocol.hpp>

#include "../handlers/character_handler.hpp"
#include "client_connection.hpp"
#include "handler.hpp"
#include "player_session.hpp"

#include <cryptography/packet_scrambler.hpp>

#include <keycap/root/compression/zip.hpp>
#include <keycap/root/network/service_locator.hpp>
#include <keycap/root/network/srp6/utility.hpp>
#include <keycap/root/utility/crc32.hpp>
#include <keycap/root/utility/random.hpp>
#include <keycap/root/utility/string.hpp>
#include <keycap/root/utility/utility.hpp>
#include <network/services.hpp>

#include <spdlog/spdlog.h>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/sha160.h>

#include <boost/endian/conversion.hpp>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;
namespace srp6 = keycap::root::network::srp6;
namespace util = keycap::root::utility;

#include <iostream>

constexpr size_t minimum_packet_size = sizeof(uint16) + sizeof(uint32); // size + opcode
constexpr size_t maximum_packet_size = 0x2800; // the client does not support larger buffers so why should we? ;)

struct validate_result
{
    keycap::shared::network::state_result result = keycap::shared::network::state_result::abort;
    uint16 size = 0;
    keycap::protocol::client_command cmd = keycap::protocol::client_command::invalid;
};

namespace keycap::realmserver
{
    client_connection::client_connection(root::network::service_base& service, root::network::service_locator& locator)
      : connection{service}
      , auth_seed_{util::random_ui32()}
      , locator_{locator}
      , login_queue_{io_service_, scrambler_}
    {
        router_.configure_inbound(this);
    }

    validate_result validate_packet_header(net::memory_stream& stream,
                                           shared::cryptography::packet_scrambler* scrambler)
    {
        // wait for more data. do not disconnect here, since this is not an error!
        if (stream.size() < minimum_packet_size)
            return {shared::network::state_result::incomplete_data, static_cast<uint16>(stream.size()),
                    protocol::client_command::invalid};

        if (scrambler)
            scrambler->decrypt(stream);

        auto size = boost::endian::endian_reverse(stream.peek<uint16>());
        auto opcode = stream.peek<protocol::client_command>(sizeof(uint16));

        if (size > maximum_packet_size)
            return {shared::network::state_result::abort, size, opcode};

        if (size > stream.size())
            return {shared::network::state_result::incomplete_data, size, opcode};

        return {shared::network::state_result::ok, size, opcode};
    }

    bool client_connection::on_data(net::data_router const& router, net::service_type service,
                                    std::vector<uint8_t> const& data)
    {
        if (data.size() > maximum_packet_size)
            return false;

        input_stream_.put(gsl::make_span(data));

        // clang-format off
        return std::visit([&](auto state)
        {
            auto logger = root::utility::get_safe_logger("connections");
            logger->debug("[client_connection] Received data in state: {}", state.name);

            try
            {
                auto [res, size, opcode] = state.on_data(*this, router, input_stream_);
                auto success = res != shared::network::state_result::abort;
                if (input_stream_.size() > 0)
                {
                    input_stream_.clear();
                    logger->error("[client_connection] Underread packet (size {} opcode {}) in state {} from user {}", size, opcode, state.name, account_name);
                    // TODO: disconnect here?
                }

                return success;
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

        return true;
    }

    bool client_connection::on_link(net::data_router const& router, net::service_type service, net::link_status status)
    {
        auto logger = root::utility::get_safe_logger("connections");

        if (status == net::link_status::Up)
        {
            logger->debug("[client_connection] New connection");
            state_ = just_connected{*this};
        }
        else
        {
            logger->debug("[client_connection] Connection closed");
            state_ = disconnected{};
        }

        return true;
    }

    uint32_t client_connection::auth_seed() const
    {
        return auth_seed_;
    }

    root::network::service_locator& client_connection::locator() const
    {
        return locator_;
    }

    client_connection::state_result client_connection::disconnected::on_data(client_connection& connection,
                                                                             net::data_router const& router,
                                                                             net::memory_stream& stream)
    {
        auto logger = root::utility::get_safe_logger("connections");
        logger->error("[client_connection::disconnected] defuq???");
        return std::make_tuple(shared::network::state_result::abort, uint16{0}, protocol::client_command::invalid);
    }

    client_connection::just_connected::just_connected(client_connection& connection)
    {
        auto seed2 = Botan::AutoSeeded_RNG().random_vec(16);
        auto seed3 = Botan::AutoSeeded_RNG().random_vec(16);

        protocol::server_challange challange;
        challange.seed = connection.auth_seed();
        std::copy(seed2.begin(), seed2.end(), challange.seed2.begin());
        std::copy(seed3.begin(), seed3.end(), challange.seed3.begin());

        auto stream = challange.encode();
        connection.send(stream);
    }

    client_connection::state_result client_connection::just_connected::on_data(client_connection& connection,
                                                                               net::data_router const& router,
                                                                               net::memory_stream& stream)
    {
        auto[result, size, opcode] = validate_packet_header(stream, nullptr);
        if (result != shared::network::state_result::ok)
            return std::make_tuple(result, size, opcode);

        auto packet = protocol::client_session::decode(stream);
        auto addon_info = stream.get_remaining();

        protocol::request_session_key request;
        request.account_name = packet.account_name;

        auto self = std::static_pointer_cast<client_connection>(connection.shared_from_this());

        connection.locator().send_registered(shared_net::account_service_type, request.encode(), [
            =, addon_info = std::move(addon_info), client_seed = packet.client_seed, account_name = packet.account_name,
            digest = packet.digest
        ](net::service_type sender, net::memory_stream data) {
            auto reply = protocol::reply_session_key::decode(data);
            account_reply_data reply_data{account_name, client_seed, digest, addon_info};
            on_account_reply(self, reply, reply_data);
            return true;
        });

        return std::make_tuple(result, size, opcode);
    }

    void client_connection::just_connected::on_account_reply(std::weak_ptr<client_connection> connection,
                                                             protocol::reply_session_key& reply,
                                                             account_reply_data& data)
    {
        if (connection.expired())
            return;

        auto conn = connection.lock();

        auto logger = root::utility::get_safe_logger("connections");

        if (!reply.session_key)
        {
            protocol::server_auth_error session;
            // TODO: send error
            logger->info("[client_connection::just_connected] User {} tried to log in with incorrect login info!",
                         data.account_name);
            return;
        }

        auto session_key = *reply.session_key;
        Botan::BigInt K{session_key};
        if (!verify_digest(data.account_name, data.client_seed, conn->auth_seed(), K, data.digest))
        {
            protocol::server_auth_error session;
            session.result = protocol::auth_result::unknown_account;
            conn->send(session.encode());
            // TODO: send error
            logger->info("[client_connection::just_connected] User {} tried to log in with incorrect login info!",
                         data.account_name);
            return;
        }

        conn->state_ = authenticated{conn, protocol::client_addon_info::decode(data.addon_data), K};
    }

    bool client_connection::just_connected::verify_digest(std::string const& account_name, uint32 client_seed,
                                                          uint32 auth_seed, Botan::BigInt const& session_key,
                                                          std::array<uint8, 20> const& client_digest)
    {
        int zero = 0;
        Botan::SHA_1 sha1;

        sha1.update(account_name);
        sha1.update(reinterpret_cast<byte*>(&zero), sizeof(zero));
        sha1.update(reinterpret_cast<byte*>(&client_seed), sizeof(client_seed));
        sha1.update(reinterpret_cast<byte*>(&auth_seed), sizeof(auth_seed));
        sha1.update(srp6::encode_flip(session_key));
        auto digest = sha1.final();

        return std::equal(digest.begin(), digest.end(), client_digest.begin());
    }

    client_connection::authenticated::authenticated(std::shared_ptr<client_connection> connection,
                                                    protocol::client_addon_info const& client_addons,
                                                    Botan::BigInt const& session_key)
    {
        auto logger = root::utility::get_safe_logger("connections");
        logger->trace("[client_connection::authenticated] authenticated::authenticated");

        connection->scrambler_.initialize(srp6::encode_flip(session_key));

        connection->player_session_ = std::make_unique<player_session>(*connection, connection->scrambler_);

        connection->player_session_->send_addon_info(client_addons);

        connection->login_queue_.enqueue(connection->weak_from_this());
    }

    client_connection::state_result client_connection::authenticated::on_data(client_connection& connection,
                                                                              root::network::data_router const& router,
                                                                              root::network::memory_stream& stream)
    {
        auto[result, size, opcode] = validate_packet_header(stream, &connection.scrambler_);
        if (result != shared::network::state_result::ok)
            return std::make_tuple(result, size, opcode);

        auto& handlers = realmserver::get_handlers();
        auto cmd = static_cast<uint32>(opcode.get());

        if (opcode == protocol::client_command::ping)
        {
            auto packet = protocol::ping::decode(stream);
            protocol::pong answer;
            answer.counter = packet.counter;

            connection.player_session_->send(answer.encode());
        }

        character_handler ch{*connection.player_session_};

        if (auto itr = handlers.find(cmd); itr != handlers.end())
            itr->second(*connection.player_session_, stream);

        return std::make_tuple(result, size, opcode);
    }
}