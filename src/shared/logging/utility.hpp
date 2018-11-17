/*
    Copyright 2018 KeycapEmu

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

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace keycap::root::configuration
{
    class config_file;
}

namespace keycap::shared::logging
{
    struct logger_entry
    {
        std::string name;
        bool to_console = false;
        bool to_file = false;
        spdlog::level::level_enum level = spdlog::level::debug;
    };

    struct logging_entry
    {
        std::string output_directory;
        std::vector<logger_entry> loggers;
    };

    // Creates all loggers that are within the given logging_entry
    void create_loggers(logging_entry const& logging_entry);

    // Creates a single logger with the given name, the given path to where it's been logged to, the given log level, a
    // given flag wether to log to the console and a given flag wether to log to file
    void create_logger(std::string const& name, std::string const& path, bool console, bool file,
                       spdlog::level::level_enum level);

    // Loads a logging_entry from the given config_file
    logging_entry from_config_file(keycap::root::configuration::config_file const& cfg_file);
}