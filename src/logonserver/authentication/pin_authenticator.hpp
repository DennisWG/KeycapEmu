/*
    Copyright 2018-2019 KeycapEmu

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

#include <keycap/root/types.hpp>

#include <array>
#include <string_view>

namespace keycap::logonserver
{
    class pin_authenticator
    {
        static constexpr uint8_t min_pin_length = 4;  // Taken from Client's AccountLogin.lua (hardcoded)
        static constexpr uint8_t max_pin_length = 10; // Taken from Client's AccountLogin.lua
        static constexpr uint8_t salt_length_ = 16;   // Taken from Client
        static constexpr uint8_t grid_size_ = 10;     // Taken from Client

      public:
        pin_authenticator();

        // Returns the server salt
        std::array<uint8_t, salt_length_> const& server_salt();

        // Generates and returns the grid seed
        uint32_t generate_grid_seed();

        // Validates the given pin
        bool validate(std::string_view pin, std::array<uint8, 16> key_checksum,
                      std::array<uint8, 20> const& final_checksum) const;

      private:
        // Generates the pin indices from the given pin
        std::vector<byte> generate_indices(std::string_view pin) const;

        std::array<uint8_t, salt_length_> server_salt_{};

        std::array<uint8_t, grid_size_> pin_grid_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    };
}