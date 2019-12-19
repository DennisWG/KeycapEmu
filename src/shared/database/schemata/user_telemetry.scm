module keycap.shared.user_telemetry;

[keys="id, date_taken"][foreign_key="id"]
data user_telemetry
{
    [not_null][foreign_key="user(id)"][on_update="CASCADE"][on_delete="RESTRICT"]
    uint32 id;
    [not_null]
    uint64 date_taken;
    [not_null]
    string telemetry;
}