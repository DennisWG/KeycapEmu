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

#include <generated/knowledge_base.hpp>

#include <functional>
#include <optional>
#include <unordered_set>

namespace keycap::shared::database::dal
{
    struct kb_data;

    class knowledge_base_dao
    {
      public:
        virtual ~knowledge_base_dao()
        {
        }

        virtual kb_data load_data() const = 0;

        virtual std::vector<article> query_articles(std::string const& query, int category) const = 0;
    };

    struct category_data
    {
        category_data(category const& cat, std::unordered_set<sub_category> const& subs);

        category category;
        std::vector<sub_category> subcategories;
    };

    struct kb_data
    {
        std::vector<category_data> categories;
        std::vector<article> articles;
    };
}