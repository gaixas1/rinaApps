#include "app_base.h"
#include <iostream>

class testServer : public server_base {
public:
	testServer(
		const std::string & name,
		const std::string & instance,
		const std::string & dif) :
		server_base(name, instance, dif) {}
		
	int handle_flow(const int & fd) {
		std::cout << "Server - Flow created"<< std::endl;
	}
};

int main(){
	testServer("serverClient", "1", "normal.DIF");
	
	return 0;
}
