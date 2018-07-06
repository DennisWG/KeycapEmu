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
#include "PreparedStatement.hpp"

#include <mysql_connection.h>
#include <mysql_driver.h>

#include <cppconn/prepared_statement.h>

namespace Keycap::Shared::Database
{
    PreparedStatement::PreparedStatement(sql::PreparedStatement* statement, boost::asio::io_service& work_service)
      : statement_(statement)
      , work_service_(work_service)
    {
    }

    void PreparedStatement::ExecuteAsync()
    {
        parameter_index_ = 1;
        work_service_.post([&] {
            try
            {
                Execute();
            }
            catch (...)
            {
            }
        });
    }

    bool PreparedStatement::Execute()
    {
        parameter_index_ = 1;
        return statement_->execute();
    }

    std::unique_ptr<sql::ResultSet> PreparedStatement::Query()
    {
        parameter_index_ = 1;
        return std::unique_ptr<sql::ResultSet>(statement_->executeQuery());
    }

    void PreparedStatement::AddParameter(std::string const& parameter)
    {
        statement_->setString(parameter_index_++, parameter.c_str());
    }

    template <>
    void PreparedStatement::AddParameter(int param)
    {
        statement_->setInt(parameter_index_++, param);
    }

    template <>
    void PreparedStatement::AddParameter(const char* param)
    {
        statement_->setString(parameter_index_++, param);
    }
}
