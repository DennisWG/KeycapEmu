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

#include "knowledge_base.hpp"

#include "../../Database.hpp"
#include "../../prepared_statement.hpp"

namespace std
{
    template <>
    struct hash<keycap::shared::database::sub_category>
    {
        inline size_t operator()(const keycap::shared::database::sub_category& cat) const
        {
            std::hash<int> int_hasher;
            return int_hasher(cat.id) ^ int_hasher(cat.category);
        }
    };

    template <>
    struct hash<keycap::shared::database::category>
    {
        inline size_t operator()(const keycap::shared::database::category& cat) const
        {
            std::hash<int> int_hasher;
            return int_hasher(cat.id);
        }
    };

    template <>
    struct equal_to<keycap::shared::database::sub_category>
    {
        constexpr bool operator()(const keycap::shared::database::sub_category& lhs,
                                  const keycap::shared::database::sub_category& rhs) const
        {
            return lhs.id == rhs.id && lhs.category == rhs.category;
        }
    };

    template <>
    struct equal_to<keycap::shared::database::category>
    {
        constexpr bool operator()(const keycap::shared::database::category& lhs,
                                  const keycap::shared::database::category& rhs) const
        {
            return lhs.id == rhs.id;
        }
    };
}

namespace keycap::shared::database::dal
{
    class mysql_knowledge_base final : public knowledge_base_dao
    {
      public:
        mysql_knowledge_base(database& database)
          : database_{database}
        {
        }

        kb_data load_data() const override
        {
            return kb_data{load_categories(), load_articles()};
        }

        std::vector<article> query_articles(std::string const& query, int category) const override
        {
            static auto statement
                = database_.prepare_statement("SELECT * FROM article WHERE MATCH (text) AGAINST (? IN BOOLEAN MODE)");
            static auto statement_cat = database_.prepare_statement(
                "SELECT * FROM article WHERE MATCH (text) AGAINST (? IN BOOLEAN MODE) AND category = ?");

            std::unique_ptr<sql::ResultSet> result;
            if (category)
            {
                statement_cat.add_parameter(query);
                statement_cat.add_parameter(category);
                result = statement_cat.query();
            }
            else
            {
                statement.add_parameter(query);
                result = statement.query();
            }

            if (!result)
                return {};

            std::vector<article> articles;
            while (result->next())
            {
                article art{
                    result->getInt("id"),
                    result->getInt("category"),
                    result->getInt("sub_category"),
                    result->getString("subject").c_str(),
                    result->getBoolean("is_hot"),
                    result->getBoolean("is_updated"),
                    result->getString("text").c_str(),
                };

                articles.emplace_back(std::move(art));
            }

            return articles;
        }

      private:
        std::vector<category_data> load_categories() const
        {
            static auto statement = database_.prepare_statement(
                "SELECT sub_category.id as sid, sub_category.name as sname, category.id as id, category.name as name "
                "FROM sub_category "
                "LEFT JOIN category "
                "ON sub_category.category = category.id");

            auto result = statement.query();

            if (!result)
                return {};

            std::vector<category_data> data;
            std::unordered_map<category, std::unordered_set<sub_category>> categories;

            while (result->next())
            {
                auto id = result->getInt("id");

                category cat{id, result->getString("name").c_str()};
                categories[cat].insert(sub_category{result->getInt("sid"), id, result->getString("sname").c_str()});
            }

            for (auto & [ category, sub_categories ] : categories)
            {
                category_data dat(category, sub_categories);
                data.emplace_back(dat);
            }

            return data;
        }

        std::vector<article> load_articles() const
        {
            static auto statement = database_.prepare_statement("SELECT * FROM article");

            auto result = statement.query();

            if (!result)
                return {};

            std::vector<article> articles;
            while (result->next())
            {
                article art{
                    result->getInt("id"),
                    result->getInt("category"),
                    result->getInt("sub_category"),
                    result->getString("subject").c_str(),
                    result->getBoolean("is_hot"),
                    result->getBoolean("is_updated"),
                    result->getString("text").c_str(),
                };

                articles.emplace_back(std::move(art));
            }

            return articles;
        }

        database& database_;
    };

    std::unique_ptr<knowledge_base_dao> get_knowledge_base_dao(database& database)
    {
        return std::make_unique<mysql_knowledge_base>(database);
    }

    //*
    category_data::category_data(keycap::shared::database::category const& cat,
                                 std::unordered_set<sub_category> const& subs)
      : category{cat}
      , subcategories{subs.begin(), subs.end()}
    {
    }
    //*/
}
