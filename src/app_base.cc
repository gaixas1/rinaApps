#include "app_base.h"
#include <thread>
#include <string.h>
#include <unistd.h>
#include <iostream>

#define DEBUG 1

// Base APP
app_base::app_base(
	const std::string & name,
	const std::string & instance,
	const std::string & dif) :
	_name ( name), _instance(instance), _dif(dif) {
	_appl = _name + "|" + _instance;
	_timeout_sec = 1;
	_timeout_usec = 0;
	isRegistered = false;
}
app_base::~app_base() {
	if(isRegistered){
		if(rina_unregister(cfd, 
		_dif.c_str(), 
		_appl.c_str(),
		0) != 0){
#ifdef DEBUG
		std::cerr << "rina_unregister failed : return != 0" << std::endl;
#endif
		}
	}
}

void app_base::setTimeout(const int & sec, const int & usec) {
	_timeout_sec = sec;
	_timeout_usec = usec;
}

int app_base::readdata(const int & fd, byte_t * buffer) {
	size_t  rem = sizeof(mindata_t);
	ssize_t ret = read(fd, buffer, rem);
	if (ret <= 0) {
#ifdef DEBUG
		if (ret == 0) {
			std::cerr << "read () failed: return 0"<< std::endl;
		} else {
			std::cerr << "read () failed: " << strerror(errno) << std::endl;
		}
#endif
		return ret;
	}
	return readdata(fd, buffer + ret, ((mindata_t *)buffer)->size - ret);
}

int app_base::readdata(const int & fd, byte_t * buffer, const size_t  & readSize) {
	size_t  rem = readSize;
	ssize_t ret;
	do {
		ret = read(fd, buffer, rem);
		if (ret <= 0) {
#ifdef DEBUG
			if (ret == 0) {
				std::cerr << "read () failed: return 0"<< std::endl;
			} else {
				std::cerr << "read () failed: " << strerror(errno) << std::endl;
			}
#endif
			return ret;
		}
		rem -= ret;
		buffer += ret;
	} while (rem > 0);
}

int app_base::readdatatimed(const int & fd, byte_t * buffer, const int & sec, const int & usec) {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	if (select(fd + 1, &read_fds, NULL, NULL, &timeout) != 1) {
#ifdef DEBUG
		std::cerr << "readtimed () failed: timeout"<< std::endl;
#endif
		return 0;
	}
	return readdata(fd, buffer);
}

int app_base::readdatatimed(const int & fd, byte_t * buffer, const size_t  & readSize, const int & sec, const int & usec) {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	if (select(fd + 1, &read_fds, NULL, NULL, &timeout) != 1) {
#ifdef DEBUG
		std::cerr << "readtimed () failed: timeout"<< std::endl;
#endif
		return 0;
	}
	return readdata(fd, buffer, readSize);
}


// Base Client
client_base::client_base(
	const std::string & name,
	const std::string & instance,
	const std::string & servername,
	const std::string & serverinstance,
	const std::string & dif) :
	app_base(name, instance, dif), _servername(servername), _serverinstance(serverinstance) {
	_serverappl = _servername + "|" + _serverinstance;
	rina_flow_spec_unreliable(&_flow_spec);
}

int client_base::run() {
	int fd;
	fd_set fs;
/*
	cfd = rina_open();
	if (cfd < 0) {
#ifdef DEBUG
		std::cerr << "rina_open () failed: return "<< cfd << std::endl;
#endif
		return EC_RINA_OPEN;
	}
*/
	fd = rina_flow_alloc(
		(_dif == "") ? NULL : _dif.c_str(), 
		_appl.c_str(),
		_serverappl.c_str(),
		&_flow_spec,
		0);

	if (fd <= 0) {
#ifdef DEBUG
		std::cerr << "rina_flow_alloc () failed: return "<< fd << std::endl;
#endif
		return EC_FLOW_ALLOC;
	}

	handle_flow(fd);
	close(fd);
}

// Base Server
server_base::server_base(
	const std::string & name,
	const std::string & instance,
	const std::string & dif) :
	app_base(name, instance, dif) {
	waitForClose = false;
}

int server_base::run() {
	int fd;
	int selectRet;
	fd_set fs;

	cfd = rina_open();
	if (cfd < 0) {
#ifdef DEBUG
		std::cerr << "rina_open () failed: return "<< cfd << std::endl;
#endif
		return EC_RINA_OPEN;
	}

	if(rina_register(
		cfd,
		(_dif == "") ? NULL : _dif.c_str(), 
		_appl.c_str(),
		0) != 0) {
#ifdef DEBUG
		std::cerr << "rina_register () failed: return -1" << std::endl;
#endif
		return EC_RINA_REGISTER;
	}

	isRegistered = true;

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;



	closeNextLoop = false;
	numRunningThreads = 0;
	// Loop for connections
	for (;;) {
		if (closeNextLoop) {
			break;
		}

		FD_ZERO(&fs);
		FD_SET(cfd, &fs);
		selectRet = select(cfd + 1, &fs, NULL, NULL, &timeout);
		if (selectRet == 0) {
			continue;
		} 
		if (selectRet < 0) {
#ifdef DEBUG
			std::cerr << "listen  () failed: return "<< selectRet << "; stop listening" <<  std::endl;
#endif
			break;
		} 

		// New flow request
		fd = rina_flow_accept(cfd, NULL, NULL, 0);
		if (fd < 0) {
#ifdef DEBUG
			std::cerr << "rina_flow_accept () failed: return "<< fd << "; stop listening" <<  std::endl;
#endif
			break;
		}

		std::thread t(&server_base::runThread, this, fd);
		t.detach();
	}

	if (waitForClose) {
		while (numRunningThreads > 0) {
			sleep(1);
		}
	}

	return after_end();
}


void server_base::runThread(const int & fd) {
	mtx.lock();
	numRunningThreads++;
	mtx.unlock();

	int returnCode = handle_flow(fd);

	mtx.lock();
	numRunningThreads--;
	after_endThread(fd, returnCode);
	mtx.unlock();
std::cout << "Clossing flow"<< std::endl;
	close(fd);
}

int server_base::handle_flow(const int & fd) { return 0; }
int server_base::after_end() {}
void server_base::after_endThread(const int & fd, int returnCode){}
