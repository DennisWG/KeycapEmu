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

#include <keycap/root/network/memory_stream.hpp>
#include <keycap/root/types.hpp>

#include <keycap/root/cryptography/ARC4.hpp>

#include <array>
#include <vector>

namespace keycap::shared::cryptography
{
    class packet_scrambler
    {
      public:
        packet_scrambler();

        void initialize(std::vector<uint8> const& session_key);

        void decrypt(keycap::root::network::memory_stream& stream);

        void encrypt(keycap::root::network::memory_stream& stream);

      private:
        uint8 encrypt_i_ = 0;
        uint8 enrypt_j_ = 0;

        uint8 decrypt_i_ = 0;
        uint8 decrypt_j_ = 0;

        std::vector<uint8> block_;
    };
}