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

#include "login_queue.hpp"
#include <authentication.hpp>

#include <cryptography/packet_scrambler.hpp>

#include <keycap/root/network/connection.hpp>

namespace keycap::realmserver
{
    login_queue::login_queue(boost::asio::io_service& io_service, shared::cryptography::packet_scrambler& scrambler)
      : io_service_{io_service}
      , work_strand_{io_service}
      , scrambler_{scrambler}
    {
    }

    void login_queue::enqueue(std::weak_ptr<keycap::root::network::connection_base> connection)
    {
        if (connection.expired())
            return;

        io_service_.post(work_strand_.wrap([this, connection]() { enqueue_(connection); }));
    }

    void login_queue::enqueue_(std::weak_ptr<keycap::root::network::connection_base> connection)
    {
        if (connection.expired())
            return;

        queue_.push_back(connection);
        auto position = static_cast<uint32>(queue_.size());

        keycap::protocol::server_auth_wait_queue packet;
        packet.position = position;

        auto stream = packet.encode();
        scrambler_.encrypt(stream);

        connection.lock()->send(stream.buffer());

        if (position == 1)
            io_service_.post(work_strand_.wrap([this]() { login_first_connection(); }));
    }

    void login_queue::login_first_connection()
    {
        std::weak_ptr<keycap::root::network::connection_base> connection;

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);

        connection = queue_.front();
        queue_.pop_front();

        update_positions();
        if (connection.expired())
            return;

        keycap::protocol::server_auth_session packet;
        auto stream = packet.encode();
        scrambler_.encrypt(stream);

        connection.lock()->send(stream.buffer());

        if (!queue_.empty())
            io_service_.post(work_strand_.wrap([this]() { login_first_connection(); }));
    }

    void login_queue::update_positions()
    {
        uint32 position = 1;
        for (auto& connection : queue_)
        {
            if (connection.expired())
                continue;

            keycap::protocol::server_auth_wait_queue packet;
            packet.position = position;

            auto stream = packet.encode();
            scrambler_.encrypt(stream);

            connection.lock()->send(stream.buffer());
        }
    }
}