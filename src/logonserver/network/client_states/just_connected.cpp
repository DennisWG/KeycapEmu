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

#include <generated/shared_protocol.hpp>

#include <keycap/root/network/srp6/utility.hpp>

#include <spdlog/spdlog.h>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;

namespace keycap::logonserver
{
    shared::network::state_result client_connection::just_connected::on_data(client_connection& connection,
                                                                             net::data_router const& router,
                                                                             net::memory_stream& stream)
    {
        constexpr auto header_size = sizeof(byte) + sizeof(byte) + sizeof(uint16);
        constexpr auto error_position = sizeof(byte) + sizeof(byte);

        if (stream.size() < header_size || stream.peek<uint16>(error_position) > (stream.size() - header_size))
            return shared::network::state_result::incomplete_data;

        if (stream.peek<protocol::command>() != protocol::command::challange)
            return shared::network::state_result::abort;

        auto packet{protocol::client_logon_challange::decode(stream)};

        auto logger = keycap::root::utility::get_safe_logger("connections");
        logger->debug("[client_connection] {}", packet.to_string());

        if (packet.cmd != protocol::command::challange)
            return shared::network::state_result::abort;

        protocol::request_account_data request;
        request.account_name = packet.account_name;

        connection.service_locator().send_registered(
            shared_net::account_service_type, request.encode(), connection.io_service_,
            [&, account_name = packet.account_name,
             self = std::static_pointer_cast<client_connection>(connection.shared_from_this())](
                net::service_type sender, net::memory_stream data) {
                self->io_service_.post(
                    [this, self, account_name, reply = protocol::reply_account_data::decode(data)]() {
                        on_account_reply(self, reply, account_name);
                    });
                return true;
            });

        return shared::network::state_result::ok;
    }

    void client_connection::just_connected::on_account_reply(std::weak_ptr<client_connection> connection,
                                                             protocol::reply_account_data const& reply,
                                                             std::string const& account_name)
    {
        if (connection.expired())
            return;

        auto conn = connection.lock();

        auto logger = keycap::root::utility::get_safe_logger("connections");

        if (!reply.data)
        {
            conn->send_error(protocol::grunt_result::unknown_account);
            logger->info("User {} tried to log in with incorrect login info!", account_name);
            return;
        }

        constexpr auto compliance = net::srp6::compliance::Wow;
        auto parameter = net::srp6::get_parameters(net::srp6::group_parameters::_256);

        Botan::BigInt verifier{reply.data->verifier};
        Botan::BigInt salt{reply.data->salt};

        conn->account_name_ = account_name;

        challanged_data challanged_data;
        challanged_data.server = std::make_shared<net::srp6::server>(parameter, verifier, compliance);
        challanged_data.verifier = reply.data->verifier;
        challanged_data.user_salt = salt;
        challanged_data.checksum_salt = Botan::AutoSeeded_RNG().random_vec(16);
        challanged_data.account_flags = static_cast<protocol::account_flag>(reply.data->flags);

        send_server_challange(conn, challanged_data, compliance, parameter, salt, reply.data->security_options);
        conn->state_ = challanged{challanged_data};
    }

    void client_connection::just_connected::send_server_challange(std::shared_ptr<client_connection> conn,
                                                                  challanged_data const& challanged_data,
                                                                  net::srp6::compliance compliance,
                                                                  net::srp6::group_parameter const& parameter,
                                                                  Botan::BigInt const& salt, uint8 security_options)
    {
        protocol::server_logon_challange outPacket{};
        outPacket.error = protocol::grunt_result::success;
        outPacket.B = net::srp6::to_array<32>(challanged_data.server->public_ephemeral_value(), compliance);
        outPacket.g_length = 1;
        outPacket.g = static_cast<uint8_t>(parameter.g);
        outPacket.N_length = 32;
        outPacket.N = net::srp6::to_array<32>(Botan::BigInt{parameter.N}, compliance);
        outPacket.s = net::srp6::to_array<32>(salt, compliance);
        outPacket.security_flags = protocol::security_flag{security_options};
        std::copy(std::begin(challanged_data.checksum_salt), std::end(challanged_data.checksum_salt),
                  std::begin(outPacket.checksum_salt));

        if (outPacket.security_flags.test_flag(protocol::security_flag::pin))
        {
            protocol::pin pin;
            pin.pin_value = conn->authenticator_.generate_grid_seed();
            pin.pin_salt = conn->authenticator_.server_salt();
            outPacket.pin = pin;
        }
        if (outPacket.security_flags.test_flag(protocol::security_flag::matrix))
        {
            protocol::matrix matrix{};

            // sub_0041DD70 is responsible for choosing coordinates between two rounds.
            // frist time around it just does:
            //      SecurityMatrix_currentColumn = v5 % this->num_matrix_cols;
            //      SecurityMatrix_currentRow = v5 / this->num_matrix_cols;
            // where v5 is just number_to_pick. After that it gets weird and v5 changes every
            // time to an unkown value that gets calculated from number_to_pick (since changing
            // it also changes the following coordinates)
            // Speculation:
            // I believe the this->field_1F4 array is being calculated in sub_0041E120
            // See https://github.com/DennisWG/KeycapEmu/issues/3

            matrix.num_coloumns = 8;
            matrix.num_rows = 2;
            matrix.unk1 = 5; // Todo: no idea what this does. See https://github.com/DennisWG/KeycapEmu/issues/3
            matrix.num_values_to_enter = 5;
            matrix.number_to_pick = 252;

            outPacket.matrix = matrix;
        }
        if (outPacket.security_flags.test_flag(protocol::security_flag::token))
        {
            protocol::token token{};
            token.unk = 200;
            outPacket.token = token;
        }

        conn->send(outPacket.encode());
    }
}