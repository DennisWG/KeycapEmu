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

set(LIBRARY_NAME keycapemushared)

add_library(${LIBRARY_NAME}
    Cli/Handler.cpp
)

target_link_libraries(${LIBRARY_NAME}
    PUBLIC
        "${MySQL_C_Connector_ROOT_DIR}/lib/libmysql.lib"
)

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${KeycapRoot_INCLUDE_DIR}
        ${Spdlog_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
        ${AMY_INCLUDE_DIR}
        "${MySQL_C_Connector_ROOT_DIR}/include"
        "."
    PRIVATE
)

target_compile_definitions(${LIBRARY_NAME}
    PUBLIC
        USE_BOOST_ASIO=1
)

flatmessage_compile_file("Database/Schemata/User.scm" "${PROJECT_SOURCE_DIR}/templates/db_object.template" "hpp")

flatmessage_compile_file("Database/Schemata/User.scm" "${PROJECT_SOURCE_DIR}/templates/db_mysql.template" "sql")