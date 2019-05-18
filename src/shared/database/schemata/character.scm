module keycap.shared.character;

[keys="realm, `character`"][foreign_key="realm"]
data realm_character
{
    [not_null][foreign_key="realm(id)"][on_update="CASCADE"][on_delete="CASCADE"]
    uint8 realm;

    [not_null][foreign_key="`character`(`id`)"][on_update="CASCADE"][on_delete="CASCADE"]
    uint32 character;
    
    [not_null][foreign_key="user(id)"][on_update="CASCADE"][on_delete="CASCADE"]
    uint32 account;
}

data character_item
{
    [primary] [not_null] [increment]
    uint64 id;

    [not_null][foreign_key="`character`(`id`)"][on_update="CASCADE"][on_delete="CASCADE"]
    uint32 character;

    [not_null][foreign_key="item_template(id)"][on_update="CASCADE"][on_delete="CASCADE"]
    uint32 item;
}

[skip_foreign_keys_comma]
data character
{
    [primary] [not_null] [increment]
    uint32 id;

    [not_null][zero_terminated]
    string name;
    
    [not_null]
    uint8 race;
    
    [not_null]
    uint8 player_class;
    
    [not_null]
    uint8 gender;
    
    [not_null]
    uint8 skin;
    
    [not_null]
    uint8 face;
    
    [not_null]
    uint8 hair_style;
    
    [not_null]
    uint8 hair_color;
    
    [not_null]
    uint8 facial_hair;
    
    [not_null]
    uint8 level;
    
    [not_null]
    uint32 zone;
    
    [not_null]
    uint32 map;
    
    [not_null]
    float x;
    
    [not_null]
    float y;
    
    [not_null]
    float z;
    
    [not_null]
    uint32 guild;
    
    [not_null]
    uint32 flags;
    
    [not_null]
    uint8 first_login;
    
    [not_null]
    uint32 active_pet;

    repeated character_item items;
}