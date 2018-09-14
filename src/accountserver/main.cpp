#include "connection.hpp"

#include <cli/helpers.hpp>
#include <rbac/role.hpp>
#include <keycap/root/configuration/config_file.hpp>
#include <keycap/root/utility/scope_exit.hpp>
#include <keycap/root/utility/utility.hpp>

#include <database/database.hpp>
#include <version.hpp>

#include <spdlog/spdlog.h>

void create_logger(std::string const& name, bool console, bool file, spdlog::level::level_enum level)
{
    std::vector<spdlog::sink_ptr> sinks;
    if (console)
        sinks.emplace_back(std::move(std::make_shared<spdlog::sinks::stdout_sink_mt>()));
    if (file)
        sinks.emplace_back(std::move(std::make_shared<spdlog::sinks::daily_file_sink_mt>(name, 23, 59)));
    auto combined_logger = std::make_shared<spdlog::logger>(name, std::begin(sinks), std::end(sinks));
    combined_logger->set_level(level);
    spdlog::register_logger(combined_logger);
}

void create_loggers()
{
    create_logger("console", true, true, spdlog::level::debug);
    create_logger("command", false, true, spdlog::level::debug);
    create_logger("connections", true, true, spdlog::level::debug);
    create_logger("database", true, true, spdlog::level::debug);
}

struct config
{
    struct
    {
        std::string bind_ip;
        int16_t port;
        int threads;
    } network;

    struct
    {
        std::string host;
        int16_t port;
        std::string user;
        std::string password;
        std::string schema;
        int threads;
    } database;
};

config parse_config(std::string configFile)
{
    keycap::root::configuration::config_file cfgFile{configFile};

    config conf;
    conf.network.bind_ip = cfgFile.get_or_default<std::string>("Network", "BindIp", "127.0.0.1");
    conf.network.port = cfgFile.get_or_default<int16_t>("Network", "Port", 3724);
    conf.network.threads = cfgFile.get_or_default<int>("Network", "Threads", 1);

    conf.database.host = cfgFile.get_or_default<std::string>("Database", "Host", "127.0.0.1");
    conf.database.port = cfgFile.get_or_default<int16_t>("Database", "Port", 3306);
    conf.database.user = cfgFile.get_or_default<std::string>("Database", "User", "root");
    conf.database.password = cfgFile.get_or_default<std::string>("Database", "Password", "");
    conf.database.schema = cfgFile.get_or_default<std::string>("Database", "Schema", "");
    conf.database.threads = cfgFile.get_or_default<int>("Database", "Threads", 1);

    return conf;
}

boost::asio::io_service& get_db_service()
{
    static boost::asio::io_service db_service;
    return db_service;
}

keycap::shared::database::database& get_login_database()
{
    static keycap::shared::database::database login_database{get_db_service()};
    return login_database;
}

void init_databases(std::vector<std::thread>& thread_pool, config const& config)
{
    get_login_database().connect(config.database.host, config.database.port, config.database.user,
                                 config.database.password, config.database.schema);

    auto& service = get_db_service();
    for (int i = 0; i < config.database.threads; ++i)
        thread_pool.emplace_back([&] { service.run(); });
}

void kill_databases(std::vector<std::thread>& thread_pool)
{
    get_db_service().stop();
    for (auto& thread : thread_pool)
    {
        if (thread.joinable())
            thread.join();
    }
}

keycap::shared::cli::command_map commands;

auto& get_command_map()
{
    return commands;
}

void register_command(keycap::shared::cli::command const& command)
{
    commands[command.name] = command;
}

keycap::shared::rbac::permission_set get_all_permissions()
{
    auto const& perms = keycap::shared::permission::to_vector();
    return keycap::shared::rbac::permission_set{std::begin(perms), std::end(perms)};
}

int main()
{
    namespace utility = keycap::root::utility;

    utility::set_console_title("Accountserver");

    create_loggers();
    QUICK_SCOPE_EXIT(sc, [] { spdlog::drop_all(); });

    auto console = utility::get_safe_logger("console");
    console->info("Running KeycapEmu.Accountserver version {}/{}", keycap::emu::version::GIT_BRANCH,
                  keycap::emu::version::GIT_HASH);
    auto config = parse_config("account.json");

    console->info("Listening to {} on port {} with {} thread(s).", config.network.bind_ip, config.network.port,
                  config.network.threads);

    boost::asio::io_service::work db_work{get_db_service()};
    std::vector<std::thread> db_thread_pool;
    init_databases(db_thread_pool, config);
    SCOPE_EXIT(sc2, [&] { kill_databases(db_thread_pool); });

    bool running = true;

    using namespace std::string_literals;
    using keycap::shared::permission;
    using keycap::shared::cli::command;
    namespace rbac = keycap::shared::rbac;
    register_command(command{"shutdown"s, permission::CommandShutdown,
                             [&running](std::vector<std::string> const& args, rbac::role const& role) {
                                 running = false;
                                 return true;
                             },
                             "Shuts down the Server"s});

    keycap::accountserver::account_service service{config.network.threads};
    service.start(config.network.bind_ip, config.network.port);

    keycap::shared::cli::run_command_line(keycap::shared::rbac::role{0, "Console", get_all_permissions()}, running);
}