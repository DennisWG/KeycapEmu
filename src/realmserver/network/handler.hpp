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

#include <generated/client.hpp>

#include <array>
#include <functional>
#include <unordered_map>

namespace keycap::root::network
{
    class memory_stream;
}

namespace keycap::realmserver
{
    class player_session;

    namespace handler::impl
    {
        template <class T, class = void>
        struct has_decode
        {
        };

        template <class T>
        struct has_decode<T, decltype(T::decode, void())>
        {
            typedef void type;
        };

        template <class T, class = void>
        struct has_decode_method
        {
            static const bool value = false;
        };

        template <class T>
        struct has_decode_method<T, typename has_decode<T>::type>
        {
            static const bool value = true;
        };

        template <typename assertion>
        struct assert_value
        {
            static_assert(assertion::value, "Assertion failed <see below for more information>");
            static bool const value = assertion::value;
        };

        template <typename RETURN_T>
        RETURN_T extract(player_session& session, keycap::root::network::memory_stream& stream)
        {
            // Automagically decode messages if they have a `decode` method
            if constexpr (has_decode_method<RETURN_T>::value)
                return RETURN_T::decode(stream);
            else
                static_assert(assert_value<has_decode_method<RETURN_T>>::value,
                              "No overload for your type was found! Declare one!"); //*/
            return RETURN_T{};
        }

        template <typename T, typename RETURN_T, typename... ARGS_T>
        std::tuple<ARGS_T...> function_args(player_session& session, keycap::root::network::memory_stream& stream,
                                            RETURN_T (T::*func)(ARGS_T...))
        {
            using tuple_type = std::tuple<ARGS_T...>;
            return std::tuple<ARGS_T...>(extract<ARGS_T>(session, stream)...);
        }

        struct command_hasher
        {
            std::size_t operator()(const keycap::protocol::client_command& key) const
            {
                return std::hash<uint32>()(static_cast<uint32>(key.get()));
            }
        };

        template <typename T, typename RETURN_T, typename... ARGS_T>
        RETURN_T proxy_call(T* object, RETURN_T (T::*func)(ARGS_T...), ARGS_T&&... args)
        {
            return (object->*func)(std::forward<ARGS_T>(args)...);
        }
    }

    struct command_handler
    {
        keycap::protocol::client_command command;

        template <typename CALLBACK_T, typename HANDLER_T>
        command_handler(keycap::protocol::client_command command, CALLBACK_T callback, HANDLER_T* handler)
          : command(command)
        {
            wrapper_ = [](void* handler, void* callback, player_session& session,
                          keycap::root::network::memory_stream& stream) -> bool {
                constexpr CALLBACK_T dummy_callback{nullptr};

                auto arg_tmp = std::make_tuple(reinterpret_cast<HANDLER_T*>(handler));
                auto arguments = std::tuple_cat(arg_tmp, handler::impl::function_args(session, stream, dummy_callback));

                return std::apply(*reinterpret_cast<CALLBACK_T*>(&callback), std::move(arguments));
            };

            // TODO: This has to be the most ugliest hack I've ever done! Is this even valid C++? Look for an
            // actual solution to this ugly mess... See https://github.com/DennisWG/KeycapEmu/issues/10
            callback_ = reinterpret_cast<void*>(*(reinterpret_cast<size_t*>(&callback)));
            handler_ = reinterpret_cast<void*>(handler);
        }

        bool operator()(player_session& session, keycap::root::network::memory_stream& stream)
        {
            return wrapper_(handler_, callback_, session, stream);
        }

      private:
        using wrapper_t = bool(void*, void*, player_session&, keycap::root::network::memory_stream&);

        wrapper_t* wrapper_{nullptr};
        void* callback_{nullptr};
        void* handler_{nullptr};
    };

    using handler_container = std::unordered_map<uint32, command_handler>;
    handler_container& get_handlers();
}

#define keycap_add_handler(command, handler, callback)                                                                 \
    handlers.try_emplace(static_cast<uint32>(command),                                                                 \
                         keycap::realmserver::command_handler{command, callback, handler});