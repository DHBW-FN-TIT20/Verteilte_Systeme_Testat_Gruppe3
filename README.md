# Verteilte_Systeme_Testat_Gruppe3
Es soll ein Publisher/Subscriber System im Rahmen der verteilte Systeme Vorlesung erstellt werden.
# client Libraries
- TCLAP: Templatized C++ Command Line Parser Library (https://tclap.sourceforge.net/)
# Client Compilation
- g++ client.cpp -o client.exe -lstdc++ -lws2_32 -I../libraries/tclap/include
# Server Compilation
- g++ server.cpp topic.cpp -o server.exe -lstdc++ -lws2_32
# Functions
# Hintergrund
Diese Programm ist im Rahmen des "Verteilte Systeme" Kurses an der DHBW Ravensburg Campus Friedrichshafen entstanden.
Entwickler:
- Jasper Bärhausen
- Moritz Pacius
- Nico Bayer
- Jan Brutscher

## Ausführung und Kompilierung Client
- Kompilieren mit 
g++ client.cpp -o client.exe -lstdc++ -lws2_32 -I../libraries/tclap/include
- Ausführen mit
./client

## Ausführung und Kompilierung Server
- Kompilieren mit
g++ server.cpp topic.cpp -o server.exe -lstdc++ -lws2_32
- Ausführen mit 
./server

## Farbenidentifikation bei Client
Blau Antwort vom Server
Rot Error
Grün Info

## Farbenidentifikation bei Server
Blau Antwort an einen Client
Rot Error
Grün Info
Cyan Nachrichten vom Client


