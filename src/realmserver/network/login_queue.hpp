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

#pragma once

#include <boost/asio.hpp>

#include <deque>
#include <memory>
#include <mutex>

namespace keycap::root::network
{
    class connection_base;
}

namespace keycap::shared::cryptography
{
    class packet_scrambler;
}

namespace keycap::realmserver
{
    // Enqueues newly authenticated connections before they're logged in.
    class login_queue
    {
      public:
        login_queue(boost::asio::io_service& io_service, shared::cryptography::packet_scrambler& scrambler);

        // Enqueues the given connection
        void enqueue(std::weak_ptr<keycap::root::network::connection_base> connection);

      private:
        void enqueue_(std::weak_ptr<keycap::root::network::connection_base> connection);

        // Logs the first connection in the queue. Will then update all clients with their new positions.
        void login_first_connection();

        // Updates all clients with their new positions.
        void update_positions();

        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand work_strand_;

        std::deque<std::weak_ptr<keycap::root::network::connection_base>> queue_;

        shared::cryptography::packet_scrambler& scrambler_;
    };
} // namespace keycap::realmserver