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

message reply_account_data
{
    protocol cmd = "protocol::reply_account_data";
    string verifier;
    string salt;
}
