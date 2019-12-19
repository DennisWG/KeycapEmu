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

#include "packet_scrambler.hpp"

#include <botan/hmac.h>

namespace crypto = keycap::root::cryptography;

namespace keycap::shared::cryptography
{
    packet_scrambler::packet_scrambler()
    {
    }

    void packet_scrambler::initialize(std::vector<uint8> const& session_key)
    {
        constexpr std::array<uint8, 16> seed{0x38, 0xA7, 0x83, 0x15, 0xF8, 0x92, 0x25, 0x30,
                                             0x71, 0x98, 0x67, 0xB1, 0x8C, 0x4,  0xE2, 0xAA};

        auto hmac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-1)");
        hmac->set_key(seed.data(), seed.size());
        hmac->start();
        hmac->update(session_key);
        block_ = std::move(hmac->final_stdvec());
    }

    void packet_scrambler::decrypt(keycap::root::network::memory_stream& stream)
    {
        constexpr uint8 decrypt_len = 6;
        if (stream.size() < decrypt_len)
            return; // TODO: throw? See https://github.com/DennisWG/KeycapEmu/issues/8

        for (uint8 i = 0; i < decrypt_len; ++i)
        {
            //*
            decrypt_i_ %= block_.size();
            uint8 x = (stream[i] - decrypt_j_) ^ block_[decrypt_i_];
            ++decrypt_i_;
            decrypt_j_ = stream[i];
            stream[i] = x;
            //*/
        }
    }

    void packet_scrambler::encrypt(keycap::root::network::memory_stream& stream)
    {
        constexpr uint8 encrypt_len = 4;
        if (stream.size() < encrypt_len)
            return; // TODO: throw? See https://github.com/DennisWG/KeycapEmu/issues/8

        for (uint8 i = 0; i < encrypt_len; ++i)
        {
            encrypt_i_ %= block_.size();
            uint8 x = (stream.data()[i] ^ block_[encrypt_i_]) + enrypt_j_;
            ++encrypt_i_;
            stream.data()[i] = enrypt_j_ = x;
        }
    }
}