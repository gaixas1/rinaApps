#include <cstdint>
#include <rina/api.h>
#include <iostream>


int main(){
	int cfd = 0;
	cfd = rina_open();
	std::cout << "cfd = "<< cfd << std::endl;
	std::cin.get();
	return 0;
}
