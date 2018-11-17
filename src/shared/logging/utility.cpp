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

#include "utility.hpp"

#include <keycap/root/configuration/config_file.hpp>

#include <filesystem>

namespace keycap::shared::logging
{
    void create_loggers(logging_entry const& logging_entry)
    {
        if (!std::filesystem::exists(logging_entry.output_directory))
            std::filesystem::create_directory(logging_entry.output_directory);

        for (auto& logger : logging_entry.loggers)
            logging::create_logger(logger.name, logging_entry.output_directory, logger.to_console, logger.to_file,
                                   logger.level);
    }

    void create_logger(std::string const& name, std::string const& path, bool console, bool file,
                       spdlog::level::level_enum level)
    {
        std::vector<spdlog::sink_ptr> sinks;
        if (console)
            sinks.emplace_back(std::move(std::make_shared<spdlog::sinks::stdout_sink_mt>()));
        if (file)
            sinks.emplace_back(
                std::move(std::make_shared<spdlog::sinks::daily_file_sink_mt>(path + '/' + name, 23, 59)));
        auto combined_logger = std::make_shared<spdlog::logger>(name, std::begin(sinks), std::end(sinks));
        combined_logger->set_level(level);
        spdlog::register_logger(combined_logger);
    }

    logging_entry from_config_file(keycap::root::configuration::config_file const& cfg_file)
    {
        namespace conf = keycap::root::configuration;

        logging_entry entry;

        entry.output_directory = cfg_file.get_or_default<std::string>("Logging", "OutputDirectory", ".");
        cfg_file.iterate_array("Logging", "Loggers", [&](conf::config_entry&& value) {
            entry.loggers.emplace_back(logging::logger_entry{
                value.get<std::string>("", "Name"),
                value.get<bool>("", "ToConsole"),
                value.get<bool>("", "ToFile"),
                static_cast<spdlog::level::level_enum>(value.get<int>("", "Level")),
            });
        });

        return entry;
    }
}