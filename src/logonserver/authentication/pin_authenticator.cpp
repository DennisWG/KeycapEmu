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

#include "pin_authenticator.hpp"

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/sha160.h>

#include <random>

namespace keycap::logonserver
{
    pin_authenticator::pin_authenticator()
    {
        static thread_local std::random_device random_device;
        static thread_local std::mt19937_64 generator{random_device()};

        std::shuffle(pin_grid_.begin(), pin_grid_.end(), generator);

        auto server_salt = Botan::AutoSeeded_RNG().random_vec(salt_length_);
        std::reverse_copy(server_salt.begin(), server_salt.end(), server_salt_.begin());
        // std::copy(server_salt.begin(), server_salt.end(), server_salt_.begin());
    }

    std::array<uint8_t, pin_authenticator::salt_length_> const& pin_authenticator::server_salt()
    {
        return server_salt_;
    }

    uint32_t pin_authenticator::generate_grid_seed()
    {
        uint32 modulate = 1;
        uint32 grid_seed = 0;

        auto remapped_grid = pin_grid_;

        for (uint32 i = 0; i < grid_size_; ++i)
        {
            auto num = remapped_grid[i];
            for (uint32 j = 0; j < grid_size_; ++j)
            {
                if (i != j && num < remapped_grid[j])
                {
                    remapped_grid[j] -= 1;
                }
            }

            grid_seed += num * modulate;
            modulate *= grid_size_ - i;
        }

        return grid_seed;
    }

    std::vector<byte> pin_authenticator::generate_indices(std::string_view pin) const
    {
        std::vector<byte> pin_indices(6, 0);
        int i = 0;

        while (!pin.empty())
        {
            byte cur_digit = pin.at(0) - 0x30;

            auto itr = std::find(pin_grid_.begin(), pin_grid_.end(), cur_digit);
            pin_indices[i++] = std::distance(pin_grid_.begin(), itr) + 0x30;

            pin.remove_prefix(1);
        }

        return pin_indices;
    }

    bool pin_authenticator::validate(std::string_view pin, std::array<uint8, 16> key_checksum,
                                     std::array<uint8, 20> const& final_checksum) const
    {
        auto pin_indices = generate_indices(pin);

        Botan::SHA_1 sha;
        sha.update(server_salt_.data(), server_salt_.size());
        sha.update(pin_indices);
        auto hash = sha.final();

        sha.update(key_checksum.data(), key_checksum.size());
        sha.update(hash.data(), hash.size());
        auto final_computed_checksum = sha.final();

        return std::equal(final_checksum.begin(), final_checksum.end(), final_computed_checksum.begin());
    }
}