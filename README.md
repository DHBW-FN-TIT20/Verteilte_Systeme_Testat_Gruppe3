# Verteilte_Systeme_Testat_Gruppe3
Es soll ein Publisher/Subscriber System im Rahmen der verteilte Systeme Vorlesung erstellt werden.
# client Libraries
- TCLAP: Templatized C++ Command Line Parser Library (https://tclap.sourceforge.net/)
# Client Compilation
- g++ client.cpp -o client.exe -lstdc++ -lws2_32 -I../libraries/tclap/include
# Functions
## client.exe --list
Fordert Liste aller Topics vom Server an
An Server: "$l"
## client.exe --topic TOPIC --topic TOPIC2 etc.
Abonniert die neuesten Informationen der jeweiligen Topics
An Server: "$t TOPIC1;TOPIC2;..."
## client.exe --publish TOPIC§NACHRICHT --publish TOPIC2§NAHRICHT2 etc.
Veröffentlicht eine Nachricht auf dem jeweilgen Topic
An Server: "$p TOPIC§NACHRICHT;TOPIC2§NACHRICHT2;..."

- Notiz von Nico: Ich erstelle zum Schluss noch ein makefile
