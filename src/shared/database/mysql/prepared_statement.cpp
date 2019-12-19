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
#include "prepared_statement.hpp"

#include <mysql_connection.h>
#include <mysql_driver.h>

#include <cppconn/prepared_statement.h>

namespace keycap::shared::database
{
    prepared_statement::prepared_statement(sql::PreparedStatement* statement, boost::asio::io_service& work_service)
      : statement_(statement)
      , work_service_(work_service)
    {
    }

    void prepared_statement::execute_async()
    {
        parameter_index_ = 1;
        work_service_.post([&] {
            try
            {
                execute();
            }
            catch (...)
            {
            }
        });
    }

    bool prepared_statement::execute()
    {
        parameter_index_ = 1;
        return statement_->execute();
    }

    std::unique_ptr<sql::ResultSet> prepared_statement::query()
    {
        parameter_index_ = 1;
        return std::unique_ptr<sql::ResultSet>(statement_->executeQuery());
    }

    void prepared_statement::add_parameter(std::string const& parameter)
    {
        statement_->setString(parameter_index_++, parameter.c_str());
    }

    template <>
    void prepared_statement::add_parameter(int param)
    {
        statement_->setInt(parameter_index_++, param);
    }

    template <>
    void prepared_statement::add_parameter(uint8_t param)
    {
        statement_->setUInt(parameter_index_++, param);
    }

    template <>
    void prepared_statement::add_parameter(uint32_t param)
    {
        statement_->setUInt(parameter_index_++, param);
    }
    
    template <>
    void prepared_statement::add_parameter(uint64_t param)
    {
        statement_->setUInt64(parameter_index_++, param);
    }

    template <>
    void prepared_statement::add_parameter(int64_t param)
    {
        statement_->setInt64(parameter_index_++, param);
    }

    template <>
    void prepared_statement::add_parameter(const char* param)
    {
        statement_->setString(parameter_index_++, param);
    }
}
