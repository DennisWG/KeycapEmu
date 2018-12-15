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

add_executable(accountserver
    main.cpp
    cli/account.cpp
    cli/help.cpp
    network/connection.cpp
    ${version_file}
)

target_link_libraries(accountserver 
    keycaproot
    keycapemushared
    boost
    ${Boost_LIBRARIES}
    ${Botan_LIBRARIES}
    "${MySQL_C_Connector_ROOT_DIR}/lib/libmysql.lib"
    "${MySQL_C_Connector_ROOT_DIR}/lib/vs14/mysqlclient.lib")

target_include_directories(accountserver
    PRIVATE
        ${Botan_INCLUDE_DIR}
        ${KeycapRoot_INCLUDE_DIR}/../../contrib/json/src
)
