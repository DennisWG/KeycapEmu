module keycap.protocol.server;

protocol server;

enum server_command : word
{
    invalid = 0,
    char_enum = 59,
    auth_srp6_response = 57,

    pong = 477,

    auth_challange = 492,
    auth_session = 494,

    addon_info = 751,

    realm_split = 907,
}