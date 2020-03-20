module keycap.protocol.shared_protocol;

import keycap.protocol.network_constants;
import keycap.protocol.character_select;

protocol shared_protocol;

enum shared_command : byte
{
    request_account_data = 0,
    reply_account_data = 1,

    update_session_key = 2,
    
    request_session_key = 3,
    reply_session_key = 4,

    request_realm_data = 5,
    reply_realm_data = 6,
    realm_hello = 7,
	request_characters = 8,
	reply_characters = 9,

	request_account_id_from_name = 10,
	reply_account_id = 11,

    login_telemetry = 12,

    char_create = 13,
    reply_char_create = 14,
}

message request_account_data
{
    shared_command cmd = "shared_command::request_account_data";

    string account_name;
}

data account_data
{
    string verifier;
    string salt;
    uint8 security_options;
    uint32 flags;
}

message reply_account_data
{
    shared_command cmd = "shared_command::reply_account_data";

    optional account_data data;
}

message update_session_key
{
    shared_command cmd = "shared_command::update_session_key";

    string account_name;
    string session_key;
}

message request_session_key
{
    shared_command cmd = "shared_command::request_session_key";

    string account_name;
    string session_key;
}

message reply_session_key
{
    shared_command cmd = "shared_command::reply_session_key";

    optional string session_key;
}

message request_realm_data
{
    shared_command cmd = "shared_command::request_realm_data";

    uint8 realm_id;
}

data realm_info
{
     realm_type type;
     uint8 locked;
     realm_flag realm_flags;
     [zero_terminated]
     string name;
     [zero_terminated]
     string ip;
     float population;
     realm_category category;
     uint8 id;
}

message reply_realm_data
{
    shared_command cmd = "shared_command::reply_realm_data";
    
     realm_info info;
}

message realm_hello
{
    shared_command cmd = "shared_command::realm_hello";
    
     realm_info info;
}

message request_characters
{
	shared_command cmd = "shared_command::request_characters";

	uint8 realm_id;
	uint32 account_id;
}

message reply_characters
{
	shared_command cmd = "shared_command::reply_characters";

	repeated char_data characters;
}

message request_account_id_from_name
{
	shared_command cmd = "shared_command::request_account_id_from_name";

	string account_name;
}

message reply_account_id
{
	shared_command cmd = "shared_command::reply_account_id";
	
	uint32 account_id;
}

message login_telemetry
{
    shared_command cmd = "shared_command::login_telemetry";

    string account_name;
    [zero_terminated] string telemetry;
}

message char_create
{
    shared_command cmd = "shared_command::char_create";
    
	uint8 realm_id;
	uint32 account_id;
    char_data data;
}

message reply_char_create
{
    shared_command cmd = "shared_command::reply_char_create";
    char_create_result result;
}