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

#include <boost/asio.hpp>

#include <mysql_connection.h>
#include <mysql_driver.h>

#include <string>

namespace Keycap::Shared::Database
{
    class PreparedStatement;

    class Database
    {
      public:
        explicit Database(boost::asio::io_service& work_service);

        // Connects to the database with the given connection information
        void Connect(std::string const& host, uint16_t port, std::string const& username, std::string const& password,
                     std::string const& schema);

        // Prepares the given statement
        PreparedStatement PrepareStatement(std::string const& statement);

        // Returns wether the database is connected
        bool IsConnected() const;

        bool Execute(std::string const& statement) const;

      private:
        boost::asio::io_service& work_service_;
        std::unique_ptr<sql::mysql::MySQL_Driver> driver_;
        std::unique_ptr<sql::Connection> connection_;
    };
}