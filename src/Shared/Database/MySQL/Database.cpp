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

#include "Database.hpp"
#include "PreparedStatement.hpp"

namespace Keycap::Shared::Database
{
    Database::Database(boost::asio::io_service& work_service)
      : work_service_(work_service)
      , driver_(sql::mysql::get_driver_instance())
    {
    }

    void Database::Connect(std::string const& host, uint16_t port, std::string const& username,
                           std::string const& password, std::string const& schema)
    {
        connection_.reset(driver_->connect(host.c_str(), username.c_str(), password.c_str()));
        connection_->setSchema(schema.c_str());
    }

    PreparedStatement Database::PrepareStatement(std::string const& query)
    {
        return PreparedStatement(connection_->prepareStatement(query.c_str()), work_service_);
    }

    bool Database::IsConnected() const
    {
        return connection_  != nullptr && !connection_->isClosed();
    }

    bool Database::Execute(std::string const& query) const
    {
        std::unique_ptr<sql::Statement> statement{connection_->createStatement()};

        return statement->execute(query.c_str());
    }
}