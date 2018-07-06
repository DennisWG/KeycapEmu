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

#include <User.hpp>

#include <functional>
#include <optional>

namespace Keycap::Shared::Database::Dal
{
    class UserDao
    {
      public:
        virtual ~UserDao()
        {
        }

        using UserCallback = std::function<void(std::optional<Shared::Database::User>)>;

        // Retreives the user with the given username from the database and then calls the given callback
        virtual void User(std::string const& username, UserCallback callback) const = 0;

        // Creates a new user in the database from the given user
        virtual void Create(Shared::Database::User const& user) const = 0;
    };
}