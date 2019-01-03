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

#include <database/daos/knowledge_base.hpp>
#include <keycap/root/utility/schedule.hpp>

#include <crow/crow.h>

#include <thread>

namespace keycap::knowledge_base
{
    class kb_service
    {
      public:
        kb_service();

        void run();

        void stop();

      private:
        std::string get_kb_setup(const crow::request& req);

        std::string send_article_query(const crow::request& req);

        std::string send_kb_qeury(const crow::request& req);

        void refresh_callback(boost::system::error_code error);

        keycap::root::utility::schedule schedule_;
        std::unique_ptr<keycap::shared::database::dal::knowledge_base_dao> kb_dao_;
        keycap::shared::database::dal::kb_data kb_data_;

        crow::SimpleApp app_;
        std::thread app_thread_;
    };
}