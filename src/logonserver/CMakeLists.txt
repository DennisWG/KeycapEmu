#   Copyright 2018 KeycapEmu
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

add_executable (logonserver
    authentication/pin_authenticator.cpp
    network/client_states/disconnected.cpp
    network/client_states/just_connected.cpp
    network/client_states/challanged.cpp
    network/client_states/authenticated.cpp
    network/client_connection.cpp
    network/logon_service.cpp
    network/realm_service.cpp
    network/realm_connection.cpp
    main.cpp
    realm_manager.cpp
    cli/help.cpp
    ${version_file}
)

target_link_libraries(logonserver
    keycaproot
    keycapemushared
    boost
    ${Boost_LIBRARIES}
    ${Botan_LIBRARIES}
)

target_include_directories(logonserver
    PRIVATE
        ${Botan_INCLUDE_DIR}
        ${KeycapRoot_INCLUDE_DIR}/../../contrib/json/src
)