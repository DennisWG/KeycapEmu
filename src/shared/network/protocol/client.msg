module keycap.protocol.client;

protocol client;

enum client_command : dword
{
    invalid = 0,

    char_create = 54,
    char_enum = 55,
    char_delete = 56,

    player_login = 61,

    contact_list = 102,

    ping = 476,

    auth_session = 493,

    voice_session_enable = 943,

    set_active_voice_channel = 978,

    opt_out_of_loot = 1032,

    realm_split = 908,
}