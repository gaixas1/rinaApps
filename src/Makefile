#Makefile

#cp /irati/lib/librina-api.so.0 /lib

# g++ -o api_test api_test.cpp -I $(RINA_INC) -L. $(RINA_LIB)


RINA_INC=/irati/include 
RINA_LIB=/irati/lib/librina-api.so 
CC=g++

testmake: api_test.o
	$(CC) -g -Wall -o testmake api_test.o -I $(RINA_INC) -L. $(RINA_LIB)
