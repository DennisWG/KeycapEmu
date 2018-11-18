module keycap.shared.network.protocol;

protocol shared;

enum protocol : byte
{
    request_account_data = 0,
    reply_account_data = 1,

    update_session_key = 2,
    
    request_session_key = 3,
    reply_session_key = 4,
}

message request_account_data
{
    protocol cmd = "protocol::request_account_data";
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
    protocol cmd = "protocol::reply_account_data";
    optional account_data data;
}

message update_session_key
{
    protocol cmd = "protocol::update_session_key";
    string account_name;
    string session_key;
}

message request_session_key
{
    protocol cmd = "protocol::request_session_key";
    string account_name;
    string session_key;
}

message reply_session_key
{
    protocol cmd = "protocol::reply_session_key";
    optional string session_key;
}
