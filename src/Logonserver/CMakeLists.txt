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
    Connection.cpp
    main.cpp
    Cli/Help.cpp
    ${version_file}
)

target_link_libraries(logonserver keycaproot keycapemushared boost ${Boost_LIBRARIES} ${Botan_LIBRARIES})

target_include_directories(logonserver
    PRIVATE
        ${Botan_INCLUDE_DIR}
        ${Spdlog_INCLUDE_DIR}
)

include(FlatmessageCompile)

flatmessage_compile_files(
    TEMPLATE "${PROJECT_SOURCE_DIR}/templates/hpp.template" 
    EXTENSION "hpp"
    FILES
        "Protocol/Logon.msg"
        "../Shared/Permissions.scm"

)

flatmessage_compile_file("Database/Account.scm" "${PROJECT_SOURCE_DIR}/templates/db_mysql.template" "sql")