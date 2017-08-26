module Keycap.Logonserver.Protocol.LogonChallange;

protocol LogonChallange;

enum Command : byte
{
    Challange = 0,
    Proof = 1,
    RealmList = 2,
}

enum Error : byte
{
    None = 0,
}

message ClientLogonChallange
{
    Command command;
    Error error;
    uint16 size;
    char[4] game;
    byte[3] version;
    uint16 build;
    uint8[4] platform;
    uint8[4] operatingSystem;
    uint8[4] country;
    uint32 timezoneBias;
    uint8[4] ip;
    string name;
}