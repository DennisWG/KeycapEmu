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
