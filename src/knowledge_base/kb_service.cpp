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

#include "kb_servce.hpp"

#include <keycap/root/type_cast.hpp>

#include <spdlog/fmt/fmt.h>

extern keycap::shared::database::database& get_kb_database();

namespace keycap::knowledge_base
{
    kb_service::kb_service()
      : kb_dao_{keycap::shared::database::dal::get_knowledge_base_dao(get_kb_database())}
      , kb_data_{kb_dao_->load_data()}
    {
        schedule_.add(std::chrono::seconds{5}, [&](auto ec) { refresh_callback(ec); });

        CROW_ROUTE(app_, "/kb/getKBSetup.xml").methods("GET"_method)([&](const crow::request& req) {
            return get_kb_setup(req);
        });

        CROW_ROUTE(app_, "/kb/sendKBArticleQuery.xml").methods("GET"_method)([&](const crow::request& req) {
            return send_article_query(req);
        });

        CROW_ROUTE(app_, "/kb/sendKBQuery.xml").methods("GET"_method)([&](const crow::request& req) {
            return send_kb_qeury(req);
        });
    }

    void kb_service::run()
    {
        app_thread_ = std::thread{[&] { app_.port(80).multithreaded().run(); }};
    }

    void kb_service::stop()
    {
        app_.stop();
        app_thread_.join();
    }

    std::string kb_service::get_kb_setup(const crow::request& req) try
    {
        auto language_id = req.url_params.get("languageId");
        auto num_articles = lexical_cast<int>(req.url_params.get("numArticles"), 20);
        auto page_number = lexical_cast<int>(req.url_params.get("pageNumber"), 1) - 1;
        auto locale = req.url_params.get("locale");

        page_number = std::max(page_number, 0);
        num_articles = std::clamp(num_articles, 0, 20);

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Root>\n";
        ss << "<Categories>\n";
        for (auto && [ cat, subcat ] : kb_data_.categories)
        {
            ss << fmt::format("<Category id=\"{}\" caption=\"{}\">\n", cat.id, cat.name);

            ss << "<SubCategories>\n";
            for (auto&& sub : subcat)
                ss << fmt::format("<SubCategory id=\"{}\" caption=\"{}\" />\n", sub.id, sub.name);
            ss << "</SubCategories>\n";

            ss << "</Category>\n";
        }
        ss << "</Categories>\n";

        ss << fmt::format("<ArticleHeaders articleCount=\"{}\">\n", kb_data_.articles.size());
        for (int i = 0; i < num_articles; ++i)
        {
            auto offset = i + num_articles * page_number;
            if (offset >= kb_data_.articles.size())
                break;

            auto& article = kb_data_.articles[offset];

            ss << fmt::format("<Article id=\"{}\" subject=\"{}\" hotIssue=\"{}\" updated=\"{}\" />\n", article.id,
                              article.subject, article.is_hot, article.is_updated);
        }
        ss << "</ArticleHeaders>\n";

        int n = 0;
        for (auto&& article : kb_data_.articles)
        {
            if (n++ > 19)
                break;
        }
        ss << "</Root>\n";

        return ss.str();
    }
    catch (...)
    {
        return "<Root></Root>";
    }

    std::string kb_service::send_article_query(const crow::request& req) try
    {
        auto article_id = lexical_cast<int>(req.url_params.get("articleId"), -1);
        auto language_id = req.url_params.get("languageId");
        auto search_type = lexical_cast<int>(req.url_params.get("searchType"), 1);
        auto locale = req.url_params.get("locale");

        auto article = std::find_if(kb_data_.articles.begin(), kb_data_.articles.end(),
                                    [&](auto&& article) { return article.id == article_id; });

        if (article == kb_data_.articles.end())
            return std::string{""};

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Root>\n";

        ss << "<Article>\n";
        ss << fmt::format("<id>{}</id>\n", article->id);
        ss << fmt::format("<subject>{}</subject>\n", article->subject);
        ss << fmt::format("<articleText>{}</articleText>\n", article->text);
        ss << fmt::format("<hotIssue>{}</hotIssue>\n", article->is_hot);
        ss << "</Article>\n";

        ss << "</Root>\n";

        return ss.str();
    }
    catch (...)
    {
        return "<Root></Root>";
    }

    std::string kb_service::send_kb_qeury(const crow::request& req) try
    {
        auto query = req.url_params.get("searchQuery");
        if (!query)
            return get_kb_setup(req);

        auto category_id = lexical_cast<int>(req.url_params.get("categoryId"), 0);
        auto language_id = req.url_params.get("languageId");
        auto num_articles = lexical_cast<int>(req.url_params.get("numArticles"), 20);
        auto page_number = lexical_cast<int>(req.url_params.get("pageNumber"), 1) - 1;
        auto locale = req.url_params.get("locale");

        page_number = std::max(page_number, 0);
        num_articles = std::clamp(num_articles, 0, 20);

        auto articles = kb_dao_->query_articles(query, category_id);

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Root>\n";
        ss << fmt::format("<ArticleHeaders articleCount=\"{}\">\n", articles.size());
        for (int i = 0; i < num_articles; ++i)
        {
            auto offset = i + num_articles * page_number;
            if (offset >= articles.size())
                break;

            auto& article = articles[offset];

            ss << fmt::format("<Article id=\"{}\" subject=\"{}\" hotIssue=\"{}\" updated=\"{}\" />\n", article.id,
                              article.subject, article.is_hot, article.is_updated);
        }
        ss << "</ArticleHeaders>\n";
        ss << "</Root>\n";

        return ss.str();
    }
    catch (...)
    {
        return "<Root></Root>";
    }

    void kb_service::refresh_callback(boost::system::error_code error)
    {
        if (!error)
        {
            kb_data_ = kb_dao_->load_data();
            schedule_.add(std::chrono::seconds{5}, [&](auto ec) { refresh_callback(ec); });
        }
    }
}