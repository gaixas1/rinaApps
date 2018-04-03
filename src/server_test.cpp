#include <rina/api.h>
#include <iostream>
#include "app_base.h"

class testServer : public server_base {
	testServer(
		const std::string & name,
		const std::string & instance,
		const std::string & dif) :
		server_base(name, instance, dif) {}
		
	int handle_flow(const int & fd) {
		std::cout << "Server - Flow created"<< std::endl;
	}
}

int main(){
	testServer("serverClient", "1", "normal.DIF");
	
	return 0;
}