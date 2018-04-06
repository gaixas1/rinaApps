#include "app_base.h"
#include <iostream>
#include <unistd.h>

class testClient : public client_base {
public:
	testClient(
		const std::string & name,
		const std::string & instance,
		const std::string & servername,
		const std::string & serverinstance,
		const std::string & dif) :
		client_base(name, instance, servername, serverinstance, dif) {}
		
	int handle_flow(const int & fd) {
		std::cout << "Client - Flow created : fd "<< fd << std::endl;

char text[] = "I will take the Ring.";

		if(write(fd, text, 22) != 22) {
			std::cout << "Client - error sending data"<< std::endl;
		} else {
			std::cout << "Client - data sent"<< std::endl;
		}
	}
};

int main(int argc, char ** argv) {
	std::string SappName = "testServer";
	std::string SappInstance = "1";
	std::string appName = "testClient";
	std::string appInstance = "1";
	std::string dif = "loopback";
	if(argc >= 2){
		SappName = argv[1];
	}
	if(argc >= 3){
		SappInstance = argv[2];
	}
	if(argc >= 4){
		appName = argv[3];
	}
	if(argc >= 5){
		appInstance = argv[4];
	}
	if(argc >= 6){
		dif = argv[5];
	}

	testClient app(appName, appInstance, SappName, SappInstance, dif);
	app.run();
	
	return 0;
}
