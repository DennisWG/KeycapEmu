flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\logonserver\protocol\logon.msg" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\realmserver\protocol\authentication.msg" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\network\protocol.msg" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\permissions.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\hpp.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"


flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\realm.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_object.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"
flatmessage_compiler -e "hpp" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\user.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_object.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"


flatmessage_compiler -e "sql" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\realm.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_mysql.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"
flatmessage_compiler -e "sql" -i "E:\Programmieren\C++\Keycap\KeycapEmu\src\shared\database\schemata\user.scm" -t "E:\Programmieren\C++\Keycap\KeycapEmu\templates\db_mysql.template" -o "E:\Programmieren\C++\Keycap\KeycapEmu\build\x64-Debug\src"

PAUSE