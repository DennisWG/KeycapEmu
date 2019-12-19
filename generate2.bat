@echo off
echo Building hpp protocols...
flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" ^
-d "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol\network_constants.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\logonserver\protocol\logon.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\authentication.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol\server.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol\client.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\character_select.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\realm_protocol.msg" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\permissions.scm" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol\shared_protocol.msg"
echo Done!

echo Building db objects...
flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_object.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" ^
-d "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol" ^
-d "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\realm.scm"  ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\user.scm" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\knowledge_base.scm" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\character.scm"
echo Done!

echo Building sql schemata...
flatmessage_compiler -e "sql" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_mysql.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" ^
-d "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol" ^
-d "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\realm.scm" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\user.scm" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\knowledge_base.scm" ^
-i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\character.scm"
echo Done!

echo All Done!