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

/*
#include <Keycap/Root/Configuration/ConfigFile.hpp>
#include <Keycap/Root/Utility/Utility.hpp>

struct Config
{
    struct
    {
        std::string BindIp;
        int16_t Port;
        int Threads;
    } Network;

    struct
    {
        std::string Host;
        int16_t Port;
        std::string User;
        std::string Password;
        std::string Schema;
        int Threads;
    } Database;
};

Config ParseConfig(std::string configFile)
{
    Keycap::Root::Configuration::ConfigFile cfgFile{ configFile };

    Config conf;
    conf.Network.BindIp = cfgFile.GetOrDefault<std::string>("Network", "BindIp", "127.0.0.1");
    conf.Network.Port = cfgFile.GetOrDefault<int16_t>("Network", "Port", 3724);
    conf.Network.Threads = cfgFile.GetOrDefault<int>("Network", "Threads", 1);

    conf.Database.Host = cfgFile.GetOrDefault<std::string>("Database", "Host", "127.0.0.1");
    conf.Database.Port = cfgFile.GetOrDefault<int16_t>("Database", "Port", 3306);
    conf.Database.User = cfgFile.GetOrDefault<std::string>("Database", "User", "root");
    conf.Database.Password = cfgFile.GetOrDefault<std::string>("Database", "Password", "");
    conf.Database.Schema = cfgFile.GetOrDefault<std::string>("Database", "Schema", "");
    conf.Database.Threads = cfgFile.GetOrDefault<int>("Database", "Threads", 1);

    return conf;
}

*/
int main()
{
    /*
    namespace utility = Keycap::Root::Utility;

    utility::SetConsoleTitle("Accountserver");
    auto config = ParseConfig("account.json");
    */


}