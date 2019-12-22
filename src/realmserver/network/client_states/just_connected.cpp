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

#include <generated/authentication.hpp>

#include <keycap/root/network/srp6/utility.hpp>

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/sha160.h>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace srp6 = keycap::root::network::srp6;

constexpr size_t minimum_packet_size = sizeof(uint16) + sizeof(uint32); // size + opcode
constexpr size_t maximum_packet_size = 0x2800; // the client does not support larger buffers so why should we? ;)

namespace keycap::realmserver
{
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
        auto [result, size, opcode] = connection.validate_packet_header(stream, nullptr);
        if (result != shared::network::state_result::ok)
            return std::make_tuple(result, size, opcode);

        auto packet = protocol::client_session::decode(stream);
        auto addon_info = stream.get_remaining();

        protocol::request_session_key request;
        request.account_name = packet.account_name;

        auto self = std::static_pointer_cast<client_connection>(connection.shared_from_this());

        auto callback = [=, addon_info = std::move(addon_info), client_seed = packet.client_seed,
                         account_name = packet.account_name,
                         digest = packet.digest](net::service_type sender, net::memory_stream data) {
            auto reply = protocol::reply_session_key::decode(data);
            account_reply_data reply_data{account_name, client_seed, digest, addon_info};
            on_account_reply(self, reply, reply_data);
            return true;
        };

        connection.query_account_service(request.encode(), callback);

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
            // TODO: send error. See https://github.com/DennisWG/KeycapEmu/issues/11
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
            // TODO: send error. See https://github.com/DennisWG/KeycapEmu/issues/11
            logger->info("[client_connection::just_connected] User {} tried to log in with incorrect login info!",
                         data.account_name);
            return;
        }

        conn->state_ = authenticated{conn, data.account_name, protocol::client_addon_info::decode(data.addon_data), K};
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
}