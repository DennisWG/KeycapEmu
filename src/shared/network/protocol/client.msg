module keycap.protocol.client;

protocol client;

enum client_command : dword
{
    invalid = 0,

    char_enum = 55,

    ping = 476,

    auth_session = 493,

    realm_split = 908,
}