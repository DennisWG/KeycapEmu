module keycap.protocol.Auth;

protocol Auth;

import keycap.protocol.client;
import keycap.protocol.server;

message server_challange
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd = "server_command::auth_challange";

    uint32 seed;
    uint8[16] seed2;
    uint8[16] seed3;
}

message client_session
{
    [is_size][endian_reverse]
    uint16 size;
    [expects="client_command::auth_session"]
    client_command cmd;

    uint32 build;
    uint32 login_server_id;
    [zero_terminated]
    string account_name;
    uint32 client_seed;
    uint8[20] digest;
}

data client_addon_data
{
    [zero_terminated]
    string name;

    [comment="first byte in <addon name>.pub file"]
    byte unk1;

    [comment="crc 32 of the remaining <addon name>.pub's file"]
    uint32 crc_public_key;

    [comment="crc 32 of url. always 0 for blizzard's addons"]
    uint32 crc_url;
}

message client_addon_info
{
    [compressed][requires=464][size_type="uint32"]
    repeated client_addon_data addons;
}

data key_data
{
    string public_key;

    [comment="name guessed"]
    uint32 crc_public_key;
}

data server_addon_data
{
    byte unk1;

    bool has_key_data;
    [requires="has_key_data"]
    optional key_data key_data;

    string url;
}

enum auth_result : byte
{
    ok = 12,
    failed = 13,
    reject = 14,
    bad_server_proof = 15,
    unavailable = 16,
    system_error = 17,
    billing_error = 18,
    billing_expired = 19,
    version_mismatch = 20,
    unknown_account = 21,
    incorrect_password = 22,
    session_expired = 23,
    server_shutting_down = 24,
    already_logged_in = 25,
    login_server_not_found = 26,
    wait_queue = 27,






    banned = 28,
    already_online = 29,
    no_time = 30,
    db_busy = 31,
    suspended = 32,
    parental_control = 33,
    locked_enforced = 34,
}

message server_addon_info
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd = "server_command::addon_info";

    auth_result result = "auth_result::ok";
    [no_size]
    repeated server_addon_data addons;
}

enum billing_plan_flags : byte
{
    none = 0,
}

enum expansion : byte
{
    vanilla = 0,
    tbc = 1,
}

message server_auth_session
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd = "server_command::auth_session";

    auth_result result = "auth_result::ok";
    uint32 billing_time_remaining = 0;
    billing_plan_flags billing_flags = "billing_plan_flags::none";
    uint32 billing_time_rested = 0;
    expansion addon = "expansion::tbc";
}

message server_auth_error
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd = "server_command::auth_session";

    auth_result result;
}

message server_auth_wait_queue
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd = "server_command::auth_session";

    auth_result result = "auth_result::wait_queue";
    uint32 position;
}