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

#include "client_connection.hpp"
#include "player_session.hpp"

#include <cryptography/packet_scrambler.hpp>
#include <network/services.hpp>

#include <keycap/root/utility/random.hpp>

#include <spdlog/spdlog.h>

#include <boost/endian/conversion.hpp>

namespace net = keycap::root::network;
namespace shared_net = keycap::shared::network;
namespace util = keycap::root::utility;

constexpr size_t minimum_packet_size = sizeof(uint16) + sizeof(uint32); // size + opcode
constexpr size_t maximum_packet_size = 0x2800; // the client does not support larger buffers so why should we? ;)

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

    client_connection::validate_result
    client_connection::validate_packet_header(net::memory_stream& stream,
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
                    // TODO: disconnect here? See https://github.com/DennisWG/KeycapEmu/issues/12
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

    void client_connection::query_account_service(keycap::root::network::memory_stream const& message,
                                                  keycap::root::network::service_locator::registered_callback callback)
    {
        locator().send_registered(shared_net::account_service_type, message, io_service_, callback);
    }
}