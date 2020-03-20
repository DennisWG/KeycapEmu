module keycap.protocol.character_select;

protocol character_select;

import keycap.protocol.client;
import keycap.protocol.server;
import keycap.shared.character;

message client_char_enum
{
    [is_size][endian_reverse]
    uint16 size;
    [expects="client_command::char_enum"]
    client_command cmd;
}

data char_item_data
{
    uint32 display_id;
    uint8 inventory_type;
    uint32 enchantment;
}

data char_data
{
    uint64 guid;
    [zero_terminated]
    string name;
    uint8 race;
    uint8 player_class;
    uint8 gender;
    uint8 skin;
    uint8 face;
    uint8 hair_style;
    uint8 hair_color;
    uint8 facial_hair;
    uint8 level;
    uint32 zone;
    uint32 map;
    float x;
    float y;
    float z;
    uint32 guild_id;
    uint32 flags;
    uint8 first_login;
    uint32 pet_display_id;
    uint32 pet_level;
    uint32 pet_family;
    char_item_data[20] items;
}

message server_char_enum
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd="server_command::char_enum";

    repeated char_data data;
}

message client_char_create
{
    [is_size][endian_reverse]
    uint16 size;
    [expects="client_command::char_create"]
    client_command cmd;
    
    [zero_terminated]
    string name;
    uint8 race;
    uint8 player_class;
    uint8 gender;
    uint8 skin;
    uint8 face;
    uint8 hair_style;
    uint8 hair_color;
    uint8 facial_hair;
    [comment="The client just sets this to 0. There is no legit way this could be another value!"]
    [expects="0"]
    uint8 outfit;
}

enum char_create_result : byte
{
    create_in_progress = 46,
    success = 47,
    error = 48,
    failed = 49,
    name_unavailable = 50,
    disabled = 51,
    pvp_teams_violation = 52,
    server_limit = 53,
    account_limit = 54,
    server_queue = 55,
    only_existing = 56,
    expansion = 57,

    invalid_character_name = 75,
    enter_character_name = 76,
    enter_min_characters = 77,
    name_too_long = 78,
    names_can_only_contain_letters = 79,
    must_contain_one_language = 80,
    contains_mature_language = 81,
    name_unavailable2 = 82,
    cannot_use_apostrophe = 83,
    only_one_apostrophe = 84,
    same_letter_consecutively = 85,
    cannot_use_space = 86,
    two_silent_symbols = 88,
}

message server_char_create
{
    [is_size][endian_reverse]
    uint16 size;
    server_command cmd="server_command::char_create";

    char_create_result result;
}