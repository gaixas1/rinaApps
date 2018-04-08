#!/bin/sh

g++ -o DropServer DropServer.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o LogServer LogServer.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o OnOffClient OnOffClient.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread
