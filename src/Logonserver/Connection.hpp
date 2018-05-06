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

#include "Logon.hpp"
#include "LogonService.hpp"

#include <botan/bigint.h>

#include <Keycap/Root/Network/Connection.hpp>
#include <Keycap/Root/Network/MemoryStream.hpp>
#include <Keycap/Root/Network/MessageHandler.hpp>
#include <Keycap/Root/Network/Srp6/Server.hpp>

#include <iostream>
#include <variant>

namespace Keycap::Logonserver
{
    class Connection : public Keycap::Root::Network::Connection<Connection>,
                       public Keycap::Root::Network::MessageHandler
    {
        using BaseConnection = Keycap::Root::Network::Connection<Connection>;

      public:
        explicit Connection(Keycap::Root::Network::ServiceBase& service);

        bool OnData(Keycap::Root::Network::ServiceBase& service, std::vector<uint8_t> const& data) override;

        bool OnLink(Keycap::Root::Network::ServiceBase& service, Keycap::Root::Network::LinkStatus status) override;

      private:
        enum class StateResult
        {
            // We've received the packet as intended and are ready to move on to the next state
            Ok,
            // There was some kind of error in the received packet and we have to terminate the connection
            Abort,
            // We're still wating for more data to arrive from the client
            IncompleteData,
        };

        struct ChallangedData
        {
            std::shared_ptr<Keycap::Root::Network::Srp6::Server> server;
            Botan::BigInt v;
            Keycap::Root::Network::Srp6::Compliance compliance;

            std::string username;
            Botan::BigInt userSalt;
            Botan::secure_vector<uint8_t> checksumSalt;
        };

        // Connection hasn't been established yet or has been terminated
        struct Disconnected
        {
            StateResult OnData(Connection& connection, Keycap::Root::Network::ServiceBase& service,
                               Keycap::Root::Network::MemoryStream& stream);

            std::string name = "Disconnected";
        };

        // Connection was just established
        struct JustConnected
        {
            StateResult OnData(Connection& connection, Keycap::Root::Network::ServiceBase& service,
                               Keycap::Root::Network::MemoryStream& stream);

            std::string name = "JustConnected";
        };

        // Auth Challange was send to client and we're waiting for a response
        struct Challanged
        {
            Challanged() = default;
            explicit Challanged(ChallangedData const& data) : data{ data } {}
            StateResult OnData(Connection& connection, Keycap::Root::Network::ServiceBase& service,
                               Keycap::Root::Network::MemoryStream& stream);

            std::string name = "Challanged";
            ChallangedData data;
        };

        // Client send its Challange and is now authenticated
        struct Authenticated
        {
            StateResult OnData(Connection& connection, Keycap::Root::Network::ServiceBase& service,
                               Keycap::Root::Network::MemoryStream& stream);

            std::string name = "Authenticated";
        };

        std::variant<Disconnected, JustConnected, Challanged, Authenticated> state_;

        Keycap::Root::Network::MemoryStream inputStream_;
    };
}