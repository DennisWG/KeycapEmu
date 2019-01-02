data build
{
    [primary] [not_null] [increment]
    uint8 id;
    [NotNull]
    uint8 major;
    [NotNull]
    uint8 minor;
    [NotNull]
    uint8 revision;
    [NotNull]
    uint16 build;
}

data realm
{
    [primary] [not_null] [increment]
    uint8 id;
    uint8 type;
    uint8 locked;
    uint8 flags;
    string name;
    string host;
    uint16 port;
    optional build allowed_build;
    float population;
    uint8 category;
}