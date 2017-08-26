#   Copyright 2017 KeycapEmu
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

cmake_minimum_required(VERSION 3.8.0)
project(KeycapEmu)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

################################
# Git
################################

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_BRANCH --all --tags --dirty=-d)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/lib/Utility/Version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp @ONLY)
list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp Version.h)
set(version_file ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp)

################################
# Boost
################################

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.63 COMPONENTS date_time filesystem regex system REQUIRED)
add_library(boost INTERFACE IMPORTED)

set_property(TARGET boost
    PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR}
)

################################
# external submodules
################################

include_directories(${KeycapRoot_INCLUDE_DIR})
link_directories(${KeycapRoot_LIBRARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/contrib/gsl/include)

# Compile flags
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(MY_CXX_FLAGS_LIST
	/std:c++latest
    )
  string(REPLACE ";" " " MY_CXX_FLAGS "${MY_CXX_FLAGS_LIST}")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")
endif()

add_subdirectory (src)