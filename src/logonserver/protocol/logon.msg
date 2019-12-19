module keycap.protocol.Logon;

import keycap.protocol.network_constants;

protocol logon;

enum command : byte
{
    challange = 0,
    proof = 1,
    reconnect_challange = 2,
    reconnect_proof = 3,
    survey_result = 4,
    realm_list = 16,
    xfer_initiate = 48,
    xfer_data = 49,
    xfer_accept = 50,
    xfer_resume = 51,
    xfer_cancel = 52,

    grunt_auth_challange = 0,
    grunt_auth_verify = 2,
    grunt_conn_ping = 16,
    grunt_conn_pong = 17,
    grunt_hello = 32,
    grunt_prove_session = 33,
    grunt_kick = 36,
    grunt_pc_warning = 41,
}

enum grunt_result : byte
{
    success = 0,
    unk1 = 1,
    unk2 = 2,
    banned = 3,
    unknown_account = 4,
    incorrect_password = 5,
    already_online = 6,
    no_time = 7,
    db_busy = 8,
    version_invalid = 9,
    version_update = 10,
    invalid_server = 11,
    suspended = 12,
    no_access = 13,
    success_survey = 14,
    parental_control = 15,
    locked_enforced = 16,
    disconnected = 255,
}

[flags]
enum account_flag : dword
{
    none = 0,
    game_master = 1,
    no_kick = 2,
    collector = 4,
    wow_trial = 8,
    cancelled = 16,
    igr = 32,
    wholesaler = 64,
    privileged = 128,
    eu_forbid_elv = 256,
    eu_forbid_billing = 512,
    wow_restricted = 1024,
    parental_control = 2048,
    referral = 4096,
    blizzard = 8192,
    recurring_billing = 16384,
    no_elect_up = 32768,
    kr_certificate = 65536,
    expansion_collector = 131072,
    disable_voice = 262144,
    disable_voice_speak = 524288,
    referral_resurrect = 1048576,
    eu_forbid_cc = 2097152,
    open_beta_dell = 4194304,
    pro_pass = 8388608,
    pro_pass_lock = 16777216,
    pending_upgrade = 33554432,
    retail_from_trial = 67108864,
    expansion2_collector = 134217728,
    overmind_linked = 268435456,
    demos = 536870912,
    death_knight_ok = 1073741824,
}

[flags]
enum security_flag : byte
{
    none = 0,
    pin = 1,
    matrix = 2,
    token = 4,
}

message client_logon_challange
{
    [expects="command::challange"]
    command cmd;
    [expects="8"]
    uint8 protocol_version;
    uint16 size;
    char[4] game;
    byte[3] version;
    uint16 build;
    uint8[4] platform;
    uint8[4] operating_system;
    uint8[4] country;
    uint32 timezone_bias;
    uint8[4] ip;
    string account_name;
}

data pin
{
    uint32 pin_value;
    uint8[16] pin_salt;
}

data pin_response
{
    uint8[16] key_checksum;
    uint8[20] final_checksum;
}

data matrix
{
    uint8 num_coloumns;
    uint8 num_rows;
    uint8 unk1;
    uint8 num_values_to_enter;
    [comment="1 dimensional array"]
    uint64 number_to_pick;
}

data matrix_response
{
    uint8[20] unk;
}

data token
{
    uint8 unk;
}

data token_response
{
    string input;
}

message server_logon_challange
{
    command cmd = "command::challange";
    uint8 protocol_version = 0;
    grunt_result error = "grunt_result::success";
    uint8[32] B;
    uint8 g_length;
    uint8 g;
    uint8 N_length;
    uint8[32] N;
    uint8[32] s;
    uint8[16] checksum_salt;
    security_flag security_flags;

    [requires="security_flags.test_flag(security_flag::pin)"]
    optional pin pin;

    [requires="security_flags.test_flag(security_flag::matrix)"]
    optional matrix matrix;

    [requires="security_flags.test_flag(security_flag::token)"]
    optional token token;
}

message server_logon_error
{
    command cmd = "command::challange";
    uint8 protocol_version = 0;
    grunt_result result;
}

data telemetry_data
{
    uint16 unk1;
    uint32 unk2;
    uint8[4] unk3;
    uint8[20] checksum;
}

[expected_size=75]
message client_logon_proof
{
    [expects="command::proof"]
    command cmd;
    [reverse]
    uint8[32] A;
    [reverse]
    uint8[20] M1;
    uint8[20] checksum;
    repeated telemetry_data data;
    security_flag security_flags;
    
    [requires="security_flags.test_flag(security_flag::pin)"]
    optional pin_response pin_response;
    
    [requires="security_flags.test_flag(security_flag::matrix)"]
    optional matrix_response matrix_response;
    
    [requires="security_flags.test_flag(security_flag::token)"]
    optional token_response token_response;
}

message server_logon_proof
{
    command cmd = "command::proof";
    grunt_result result = "grunt_result::success";
    uint8[20] M2;
    account_flag account_flags;
    uint32 survey_id = 0;
    uint16 num_account_messages = 0;
}

[expected_size=5]
message client_realm_list
{
    [expects="command::realm_list"]
    command cmd;
    [expects="0"]
    uint32 always_zero;
}

data realm_list_data
{
     realm_type type;
     uint8 locked;
     realm_flag realm_flags;
     [zero_terminated]
     string name;
     [zero_terminated]
     string ip;
     float population;
     uint8 num_characters;
     realm_category category;
     uint8 id;
}

message server_realm_list
{
    command cmd = "command::realm_list";
    [is_size]
    uint16 size;
    uint32 alwaysZero = 0;
    [size_type="uint16"]
    repeated realm_list_data data;
    
    uint16 unk = 0;
}

message xfer_initiate
{
    command cmd = "command::xfer_initiate";

    [comment="Can either be 'Patch' or 'Survey'"]
    string action;
    uint64 filesize;
    uint8[16] md5_checksum;
}

message xfer_data
{
    command cmd = "command::xfer_data";

    [size_type="uint16"]
    repeated uint8 chunk;
}

message survey_result
{
    [expects="command::survey_result"]
    command cmd;

    uint32 survey_id;
    uint8 error;

    [compressed][size_type="uint16"]
    string data;
}