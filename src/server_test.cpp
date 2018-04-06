#include "app_base.h"
#include <iostream>
#include <unistd.h>

class testServer : public server_base {
public:
	testServer(
		const std::string & name,
		const std::string & instance,
		const std::string & dif) :
		server_base(name, instance, dif) {}
		
	int handle_flow(const int & fd) {
		std::cout << "Server - Flow created : fd "<< fd << std::endl;
char buffer [100];
		if(read(fd, buffer, 22) > 0){
std::cout << buffer << std::endl;
		} else {
			std::cout << "Error readind"<< std::endl;
		}
	}
};

int main(int argc, char ** argv){
	std::string appName = "testServer";
	std::string appInstance = "1";
	std::string dif = "loopback";
	if(argc >= 2){
		appName = argv[1];
	}
	if(argc >= 3){
		appInstance = argv[2];
	}
	if(argc >= 4){
		dif = argv[3];
	}

	testServer app(appName, appInstance, dif);
int ret = app.run();
		std::cout << "Server - ended!!! "<< ret <<  std::endl;

	return 0;
}
