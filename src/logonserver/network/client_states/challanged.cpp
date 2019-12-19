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

#include "../../authentication/pin_authenticator.hpp"
#include "../client_connection.hpp"

#include <keycap/root/cryptography/OTP.hpp>
#include <keycap/root/network/srp6/server.hpp>
#include <keycap/root/network/srp6/utility.hpp>

#include <generated/shared_protocol.hpp>

#include <botan/base32.h>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;
namespace protocol = keycap::protocol;

namespace HOTP = keycap::root::cryptography::HOTP;

namespace keycap::logonserver
{
    void update_session_key(client_connection& connection, std::string const& account_name,
                            Botan::BigInt const& session_key)
    {
        protocol::update_session_key update;
        update.account_name = account_name;
        auto key = Botan::BigInt::encode(session_key);
        update.session_key = keycap::root::utility::to_hex_string(key.begin(), key.end(), true);

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug("[client_connection] Updating session key of {} to {}", update.account_name, update.session_key);

        connection.service_locator().send_to(shared_net::account_service_type, update.encode());
    }

    std::tuple<Botan::BigInt, Botan::BigInt, Botan::BigInt>
    client_connection::challanged::generate_session_key_and_server_proof(protocol::client_logon_proof const& packet)
    {
        Botan::BigInt A{packet.A.data(), 32};
        Botan::BigInt M1{packet.M1.data(), 20};
        auto session_key = data.server->session_key(A);

        auto M1_S = net::srp6::generate_client_proof(data.server->prime(), data.server->generator(), data.user_salt,
                                                     data.username, A, data.server->public_ephemeral_value(),
                                                     session_key, data.server->compliance_mode());

        return std::make_tuple(std::move(session_key), std::move(M1), std::move(M1_S));
    }

    void client_connection::challanged::send_proof_success(client_connection& connection, Botan::BigInt session_key,
                                                           Botan::BigInt M1_S)
    {
        protocol::server_logon_proof outPacket;
        outPacket.M2 = net::srp6::to_array<20>(data.server->proof(M1_S, session_key), data.server->compliance_mode());
        outPacket.account_flags = data.account_flags;
        outPacket.num_account_messages = 0;

        connection.send(outPacket.encode());
    }

    template <typename... Args>
    shared::network::state_result login_error(char const* error_message, client_connection& connection,
                                              protocol::grunt_result result, Args&&... arguments)
    {
        auto logger = keycap::root::utility::get_safe_logger("connections");
        connection.send_error(result);
        logger->info(error_message, std::forward<Args>(arguments)...);

        return shared::network::state_result::ok;
    }

    bool validate_pin(pin_authenticator const& authenticator, protocol::pin_response const& response,
                      std::string const& totp_secret)
    {
        const auto time = std::time(nullptr);
        const auto now = static_cast<std::uint64_t>(time);

        for (int i = -1; i < 2; ++i)
        {
            auto step = static_cast<std::uint64_t>((std::floor(now / 30))) + i;
            auto code = HOTP::generate(totp_secret, step, 6);
            printf("%s %d\n", code.c_str(), std::stoi(code));
            if (authenticator.validate(code, response.key_checksum, response.final_checksum))
                return true;
        }

        return false;
    }

    shared::network::state_result client_connection::challanged::on_data(client_connection& connection,
                                                                         net::data_router const& router,
                                                                         net::memory_stream& stream)
    {
        if (stream.size() < protocol::client_logon_proof::expected_size)
            return shared::network::state_result::incomplete_data;

        if (stream.peek<protocol::command>() != protocol::command::proof)
            return shared::network::state_result::abort;

        auto packet{protocol::client_logon_proof::decode(stream)};

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug(packet.to_string());

        std::string const key = "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ";
        auto totp_secret
            = Botan::base32_encode(reinterpret_cast<uint8_t*>(data.verifier.data()), data.verifier.size());

        if (packet.pin_response)
        {
            if (!validate_pin(connection.authenticator_, *packet.pin_response, key))
            {
                return login_error("[client_connection] User {} tried to log in with incorrect PIN!", connection,
                                   protocol::grunt_result::unknown_account, data.username);
            }
        }

        auto [session_key, M1, M1_S] = generate_session_key_and_server_proof(packet);

        if (M1_S != M1)
        {
            return login_error("[client_connection] User {} tried to log in with incorrect login info!", connection,
                               protocol::grunt_result::unknown_account, data.username);
        }

        update_session_key(connection, data.username, session_key);

        send_proof_success(connection, session_key, M1_S);

        connection.state_ = authenticated{};
        return shared::network::state_result::ok;
    }
}