#include <rina/api.h>
#include <iostream>
#include "app_base.h"

class testClient : public client_base {
	testClient(
		const std::string & name,
		const std::string & instance,
		const std::string & servername,
		const std::string & serverinstance,
		const std::string & dif) :
		client_base(name, instance, servername, serverinstance, dif) {}
		
	int handle_flow(const int & fd) {
		std::cout << "Client - Flow created"<< std::endl;
	}
}

int main(){
	testClient("testClient", "1", "serverClient", "1", "normal.DIF");
	
	return 0;
}