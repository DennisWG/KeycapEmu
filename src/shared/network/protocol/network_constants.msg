module keycap.protocol.network_constants;

protocol network_constants;

enum realm_type : byte
{
    pve = 0,
    pvp = 1,
    rp = 6,
    rppvp = 8,
}

[flags]
enum realm_flag : byte
{
    none = 0,
    invalid = 1,
    offline = 2,
    specify_build = 4,
    unk1 = 8,
    unk2 = 16,
    recommended = 32,
    new_ = 64,
    full = 128,
}

enum realm_category : byte
{
    development = 1,
    united_states = 2,
    oceanic = 3,
    latin_america = 4,
    tournament = 5,
    korea = 6,
    tournament2 = 7,
    english = 8,
    german = 9,
    french = 10,
    spanish = 11,
    russian = 12,
    tournament3 = 13,
    taiwan = 14,
    tournament4 = 15,
    china = 16,
    cn1 = 17,
    cn2 = 18,
    cn3 = 19,
    cn4 = 20,
    cn5 = 21,
    cn6 = 22,
    cn7 = 23,
    cn8 = 24,
    tournament5 = 25,
    test_server = 26,
    qa_server = 27,
    tournament6 = 28,
    cn9 = 29,
    test_server_2 = 30,
}