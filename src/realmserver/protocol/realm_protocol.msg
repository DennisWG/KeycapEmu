module keycap.protocol.realm;

protocol realm;

import keycap.protocol.client;
import keycap.protocol.server;

message ping
{
    [is_size][endian_reverse]
    uint16 size;
    [expects="client_command::ping"]
    client_command cmd;

    uint32 counter;
    uint32 latency;
}

message pong
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd="server_command::pong";

    uint32 counter;
}

enum realm_split_choice : dword
{
    request = -1,
    none = 0,
    realm_1 = 1,
    realm_2 = 2,
}

enum realm_split_state : dword
{
    none = 0,
    split = 1,
    pending = 2,
}

message client_realm_split
{
    [is_size][endian_reverse]
    uint16 size;
    [expects="client_command::realm_split"]
    client_command cmd;

    realm_split_choice choice;
}

message server_realm_split
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd="server_command::realm_split";

    realm_split_choice choice = "realm_split_choice::none";
    realm_split_state state = "realm_split_state::none";
    [zero_terminated]
    string date;
}
