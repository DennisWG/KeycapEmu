data user
{
    [index] [not_null] [increment]
    uint32 id;
    string account_name;
    string email;
    uint8 security_options;
    uint32 flags;
    string verifier;
    string salt;
    string session_key;
}