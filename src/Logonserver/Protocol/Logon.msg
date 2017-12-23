module Keycap.Logonserver.Protocol.LogonChallange;

protocol Logon;

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

enum AuthResult : byte
{
    Ok = 0,
    Banned = 3,
    InvalidInfo = 4,
    AlreadyOnline = 6,
    NoTime = 7,
    DbBusy = 8,
    BadVersion = 9,
    DownloadFile = 10,
    Suspended = 12,
    LockedEnforced = 16,
    Disconnected = 255,
}

enum AccountFlags : dword
{
    None = 0,
    GameMaster = 1,
    NoKick = 2,
    Collector = 4,
    WowTrial = 8,
    Cancelled = 16,
    Igr = 32,
    Wholesaler = 64,
    Privileged = 128,
    EuForbidElv = 256,
    EuForbidBilling = 512,
    WowRestricted = 1024,
    ParentalControl = 2048,
    Referral = 4096,
    Blizzard = 8192,
    RecurringBilling = 16384,
    NoElectUp = 32768,
    KrCertificate = 65536,
    ExpansionCollector = 131072,
    DisableVoice = 262144,
    DisableVoiceSpeak = 524288,
    ReferralResurrect = 1048576,
    EuForbidCc = 2097152,
    OpenBetaDell = 4194304,
    ProPass = 8388608,
    ProPassLock = 16777216,
    PendingUpgrade = 33554432,
    RetailFromTrial = 67108864,
    Expansion2Collector = 134217728,
    OvermindLinked = 268435456,
    Demos = 536870912,
    DeathKnightOk = 1073741824,
}

enum SecurityFlags : byte
{
    None = 0,
    Pin = 1,
    Matrix = 2,
    Token = 4,
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

message ServerLogonChallange
{
    Command command;
    Error error;
    AuthResult authResult;
    uint8[32] B;
    uint8 g_length;
    uint8 g;
    uint8 N_length;
    uint8[32] N;
    uint8[32] s;
    uint8[16] checksumSalt;
    SecurityFlags securityFlags;
}

message ServerLogonProof
{
    Command command;
    Error error;
    uint8[20] M2;
    AccountFlags accountFlags;
    uint32 surveyId;
    uint16 messageFlags;
}