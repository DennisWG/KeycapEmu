module keycap.logonserver.protocol.LogonChallange;

protocol logon;

enum command : byte
{
    Challange = 0,
    Proof = 1,
    ReconnectChallange = 2,
    ReconnectProof = 3,
    SurveyResult = 4,
    RealmList = 16,
    XferInitiate = 48,
    XferData = 49,

    GruntAuthChallange = 0,
    GruntAuthVerify = 2,
    GruntConnPing = 16,
    GruntConnPong = 17,
    GruntHello = 32,
    GruntProveSession = 33,
    GruntKick = 36,
    GruntPcWarning = 41,
}

enum error : byte
{
    Success = 0,
    Unk1 = 1,
    Unk2 = 2,
    Banned = 3,
    UnknownAccount = 4,
    IncorrectPassword = 5,
    AlreadyOnline = 6,
    NoTime = 7,
    DbBusy = 8,
    VersionInvalid = 9,
    VersionUpdate = 10,
    InvalidServer = 11,
    Suspended = 12,
    NoAccess = 13,
    SuccessSurvey = 14,
    ParentalControl = 15,
}

enum auth_result : byte
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

enum account_flag : dword
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

enum security_flag : byte
{
    None = 0,
    Pin = 1,
    Matrix = 2,
    Token = 4,
}

enum realm_type : byte
{
    PvE = 0,
    PvP = 1,
    Rp = 6,
    RpPvp = 8,
}

enum realm_flag : byte
{
    None = 0,
    Invalid = 1,
    Offline = 2,
    SpecifyBuild = 4,
    Unk1 = 8,
    Unk2 = 16,
    Recommended = 32,
    New = 64,
    Full = 128,
}

message client_logon_challange
{
    [Expects="command::Challange"]
    command cmd;
    [Expects="error::DbBusy"]
    error unk_error_maybe;
    uint16 size;
    char[4] game;
    byte[3] version;
    uint16 build;
    uint8[4] platform;
    uint8[4] operating_system;
    uint8[4] country;
    uint32 timezone_bias;
    uint8[4] ip;
    string account_name;
}

message server_logon_challange
{
    command cmd = "command::Challange";
    error error_code = "error::Success";
    auth_result result;
    uint8[32] B;
    uint8 g_length;
    uint8 g;
    uint8 N_length;
    uint8[32] N;
    uint8[32] s;
    uint8[16] checksum_salt;
    security_flag security_flags;
}

message server_logon_challange_error
{
    command cmd = "command::Challange";
    error error_code = "error::Success";
    auth_result result;
}

data telemetry_data
{
    uint16 unk1;
    uint32 unk2;
    uint8[4] unk3;
    uint8[20] checksum;
}

message client_logon_proof
{
    [Expects="command::Proof"]
    command cmd;
    [Reverse=1]
    uint8[32] A;
    [Reverse=1]
    uint8[20] M1;
    uint8[20] checksum;
    repeated telemetry_data data;
    uint8 security_flags;
}

message server_logon_proof
{
    command cmd = "command::Proof";
    error error_code = "error::Success";
    uint8[20] M2;
    account_flag account_flags;
    uint32 survey_id = 0;
    uint16 num_account_messages = 0;
}

message client_realm_list
{
    [Expects="command::RealmList"]
    command cmd;
    [Expects="0"]
    uint32 always_zero;
}

data realm_list_data
{
     realm_type type;
     uint8 locked;
     realm_flag realm_flags;
     [ZeroTeminated=1]
     string name;
     [ZeroTeminated=1]
     string ip;
     float population;
     uint8 num_characters;
     uint8 category;
     uint8 id;
}

message ServerRealmList
{
    command cmd = "command::RealmList";
    [IsSize=1]
    uint16 size;

    uint32 alwaysZero = 0;
    [SizeType="uint16"]
    repeated realm_list_data data;
    
     uint16 unk = 0;
}