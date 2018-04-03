#!/bin/sh

g++ -o api_test api_test.cpp -I /irati/include/ -L. /irati/lib/librina-api.so

g++ -o client_test app_base.cc client_test.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread

g++ -o server_test app_base.cc server_test.cpp -I /irati/include/ -L. /irati/lib/librina-api.so -lpthread
