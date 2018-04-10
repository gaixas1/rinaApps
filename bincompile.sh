#!/bin/sh

g++ -o DropServer DropServer.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o LogServer LogServer.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o OnOffClient OnOffClient.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o DataClient DataClient.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o VideoClient VideoClient.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o VoiceClient VoiceClient.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

mv DropServer bin/DropServer
mv LogServer bin/LogServer
mv OnOffClient bin/OnOffClient
mv DataClient bin/DataClient
mv VideoClient bin/VideoClient
mv VoiceClient bin/VoiceClient
