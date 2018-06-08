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

#pragma once

#include <Keycap/Root/Utility/Utility.hpp>

#include <amy.hpp>
#include <spdlog/spdlog.h>

namespace Keycap::Shared::Database
{
    // Asynchronous database
    class Database
    {
      public:
        Database(boost::asio::io_service& io_service)
          : connector_(io_service)
        {
        }

        // Connects to the given database on the given host and port with the given username/password
        void Connect(std::string const& host, uint16_t port, std::string const& user, std::string const& password,
                     std::string const& database)
        {
            boost::asio::ip::tcp::resolver resolver{connector_.get_io_service()};
            auto ep = resolver.resolve({host, ""})->endpoint();
            boost::asio::ip::tcp::endpoint endpoint{ep.address(), port};

            connector_.async_connect(endpoint, {user, password}, database, amy::default_flags,
                                     [&](boost::system::error_code const& ec) {
                                         if (ec)
                                         {
                                             auto logger = Keycap::Root::Utility::GetSafeLogger("database");
                                             logger->error(ec.message());
                                             return;
                                         }

                                         connected_ = true;
                                     });
        }

        template <typename T>
        void Query(std::string const& statement, T && resultHandler)
        {
            connector_.async_query(statement, [&](boost::system::error_code const& ec)
            {
                if (ec)
                    return resultHandler(ec, amy::result_set{});

                connector_.async_store_result(resultHandler);
            });
        }

        // Returns whether the database has established a connection
        bool IsConnected() const
        {
            return connected_;
        }

      private:
        amy::connector connector_;

        bool connected_ = false;
    };
} // namespace Keycap::Shared::Database
