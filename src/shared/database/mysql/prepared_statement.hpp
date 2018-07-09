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

#include <cppconn/prepared_statement.h>

#include <memory>
#include <string>

namespace keycap::shared::database
{
    class database;

    class prepared_statement
    {
        friend class database;

      public:
        // Adds the given parameter to the parameter list
        template <typename T>
        void add_parameter(T parameter);

        // Adds the given parameter to the parameter list
        void add_parameter(std::string const& parameter);

        using execute_async_callback = std::function<void(bool)>;

        // Executes the statement asynchronously and calls the given callback from the database thread.
        // Callback must have the signature callback(bool success)
        void execute_async(execute_async_callback callback)
        {
            parameter_index_ = 1;
            work_service_.post([&, callback = std::move(callback)] {
                try
                {
                    auto success = execute();
                    callback(success);
                }
                catch (...)
                {
                    return callback(false);
                }
            });
        }

        void execute_async();

        // Executes the statement synchronously and returns wether it succeeded
        bool execute();

        using query_async_callback = std::function<void(std::unique_ptr<sql::ResultSet>)>;

        // Queries the database asynchronously and calls the given callback from the database thread.
        // Callback must have the signature callback(std::unique_ptr<sql::ResultSet> result_set)
        void query_async(query_async_callback callback)
        {
            parameter_index_ = 1;

            work_service_.post([&, callback = std::move(callback)] {
                try
                {
                    auto result = query();
                    callback(std::move(result));
                }
                catch (...)
                {
                    callback(nullptr);
                }
            });
        }

        // Queries the database synchronously and returns the result set
        std::unique_ptr<sql::ResultSet> query();

      private:
        prepared_statement(sql::PreparedStatement* statement, boost::asio::io_service& work_service);

        std::unique_ptr<sql::PreparedStatement> statement_;
        boost::asio::io_service& work_service_;

        int parameter_index_ = 1;
    };
}