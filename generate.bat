@echo off
flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\network_constants.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\logonserver\protocol\logon.msg"

flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\authentication.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\server.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\client.msg"
flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\character_select.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\server.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\client.msg"
flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\realm_protocol.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\server.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\client.msg"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\permissions.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"


flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\realm.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_object.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\user.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_object.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\knowledge_base.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_object.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"


flatmessage_compiler -e "sql" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\realm.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_mysql.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"
flatmessage_compiler -e "sql" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\user.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_mysql.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"
flatmessage_compiler -e "sql" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\knowledge_base.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_mysql.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated"



flatmessage_compiler -e "hpp" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src\generated" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\network_constants.msg" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\shared_protocol.msg"

PAUSE