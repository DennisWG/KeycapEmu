module keycap.shared.network.protocol;

protocol shared;

enum protocol : byte
{
    request_account_data = 0,
    reply_account_data = 1,
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
    bool has_data;
    optional account_data data;
}
