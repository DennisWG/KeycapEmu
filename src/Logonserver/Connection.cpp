/*
    Copyright 2017 KeycapEmu

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

#include "Connection.hpp"

namespace net = Keycap::Root::Network;

namespace Keycap::Logonserver
{
    Connection::Connection(net::ServiceBase& service)
        : BaseConnection{ service }
    {
        router_.ConfigureInbound(this);
    }

    bool Connection::OnData(Keycap::Root::Network::ServiceBase& service, std::vector<uint8_t> const& data)
    {
        inputStream_.Put(gsl::make_span(data));
        // clang-format off
        std::visit([&](auto state)
        {
            state.OnData(service, inputStream_);
            std::cout << state.name << '\n';
        }, state_);
        // clang-format on

        return true;
    }

    bool Connection::OnLink(Keycap::Root::Network::ServiceBase& service, Keycap::Root::Network::LinkStatus status)
    {
        if (status == Keycap::Root::Network::LinkStatus::Up)
            state_ = JustConnected{};
        else
            state_ = Disconnected{};

        return true;
    }
}