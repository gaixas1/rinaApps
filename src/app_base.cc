#include "app_base.h"
#include <thread>
#include <unistd.h>

// Base APP
app_base::app_base(
	const std::string & name,
	const std::string & instance,
	const std::string & dif) :
	_name ( name), _instance(instance), _dif(dif) {
	_appl = _name + "|" + _instance;
	_timeout_sec = 1;
	_timeout_usec = 0;
}

void app_base::setTimeout(const int & sec, const int & usec) {
	_timeout_sec = sec;
	_timeout_usec = usec;
}

int app_base::read(const int & fd, byte_t * buffer) {
	size_t  rem = sizeof(mindata_t);
	ssize_t ret = read(fd, buffer, rem);
	if (ret <= 0) {
#ifdef DEBUG
		if (ret == 0) {
			LOG_ERR("read() failed: return 0");
		}
		else {
			LOG_ERR("read() failed: %s", strerror(errno));
		}
#endif
		return ret;
	}
	return read(fd, buffer + ret, ((mindata_t *)buffer)->size - ret);
}

int app_base::read(const int & fd, byte_t * buffer, const size_t  & readSize) {
	size_t  rem = readSize;
	ssize_t ret;
	do {
		ret = read(fd, buffer, rem);
		if (ret <= 0) {
#ifdef DEBUG
			if (ret == 0) {
				LOG_ERR("read() failed: return 0");
			}
			else {
				LOG_ERR("read() failed: %s", strerror(errno));
			}
#endif
			return ret;
		}
		rem -= ret;
		buffer += ret;
	} while (rem > 0);
}

int app_base::readtimed(const int & fd, byte_t * buffer, const int & sec, const int & usec) {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	if (select(fd + 1, &read_fds, NULL, NULLs, &timeout) != 1) {
#ifdef DEBUG
		LOG_WARNING("readtimed() failed: timeout 0");
#endif
		return 0;
	}
	return read(fd, buffer);
}

int app_base::readtimed(const int & fd, byte_t * buffer, const size_t  & readSize, const int & sec, const int & usec) {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	if (select(fd + 1, &read_fds, NULL, NULLs, &timeout) != 1) {
#ifdef DEBUG
		LOG_WARNING("readtimed() failed: timeout 0");
#endif
		return 0;
	}
	return read(fd, buffer, readSize);
}


// Base Client
client_base::client_base(
	const std::string & name,
	const std::string & instance,
	const std::string & servername,
	const std::string & serverinstance,
	const std::string & dif) :
	_name(name), _instance(instance), _dif(dif) :
	app_base(name, instance, dif), _servername(servername), _serverinstance(serverinstance) {
	_serverappl = _servername + "|" + _serverinstance;
	rina_flow_spec_unreliable(&_flow_spec);
}

int client_base::run() {
	int cfd;
	int fd;
	fd_set fs;

	cfd = rina_open();
	if (cfd < 0) {
#ifdef DEBUG
		LOG_ERR("rina_open() failed: return < 0");
#endif
		return cfd;
	}


	fd = rina_flow_alloc(_dif.c_str(),
		_appl.c_str(),
		_serverappl.c_str(),
		&_flow_spec,
		RINA_F_NOWAIT);

	if (fd < 0) {
#ifdef DEBUG
		LOG_ERR("rina_flow_alloc() failed: return < 0");
#endif
		return fd;
	}

	/* check first flow allocation */
	FD_ZERO(&fs);
	FD_SET(fd, &fs);

	if (select(fd + 1, fs, NULL, NULL, _timeout_sec, _timeout_usec) != 1) {
#ifdef DEBUG
		LOG_ERR("rina_flow_alloc() timeout: return = 0");
#endif
		return 0;
	}
	
	fd = rina_flow_alloc_wait(fd);
	if (fd < 0) {
#ifdef DEBUG
		LOG_ERR("rina_flow_alloc_wait() failed: return < 0");
#endif
		return fd;
	}

	handle_flow(fd);
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
	int cfd;
	int tfd;
	int selectRet;
	fd_set fs;

	cfd = rina_open();
	if (cfd < 0) {
#ifdef DEBUG
		LOG_ERR("rina_open() failed: return < 0");
#endif
		return cfd;
	}

	tfd = rina_register(
		cfd,
		_dif.c_str(), 
		_appl.c_str(),
		RINA_F_NOWAIT);

	if (tfd < 0) {
#ifdef DEBUG
		LOG_ERR("rina_register() failed: return < 0");
#endif
		return tfd;
	}

	FD_ZERO(&fs);
	FD_SET(tfd, &fs);
	if (select(tfd + 1, fs, NULL, NULL, _timeout_sec, _timeout_usec) != 1)){
#ifdef DEBUG
	LOG_ERR("select() timeout on register: return -1");
#endif
		return -1;
	}

	tfd = rina_register_wait(cfd, tfd);
	if (tfd < 0) {
#ifdef DEBUG
		LOG_ERR("select() timeout on register: return < 0");
#endif
		return tfd;
	}

	close = false;
	numRunningThreads = 0;
	// Loop for connections
	for (;;) {
		if (close) {
			break;
		}

		FD_ZERO(&fs);
		FD_SET(cfd, &fs);
		selectRet = select(cfd + 1, fs, NULL, NULL, _timeout_sec, _timeout_usec) != 1);
		if (selectRet < 0) {
#ifdef DEBUG
			LOG_ERR("select() error: stop server");
#endif
			break;
		} else if (selectRet == 0) {
			continue;
		}

		// New flow request
		tfd = rina_flow_accept(cfd, NULL, NULL, RINA_F_NORESP);
		if (tfd < 0) {
#ifdef DEBUG
			LOG_ERR("rina_flow_accept() failure: return < 0");
#endif
			return tfd;
		}
		
		tfd = rina_flow_respond(cfd, tfd, 0);

		if (tfd < 0) {
#ifdef DEBUG
			LOG_ERR("rina_flow_respond	() failed: return < 0"); ??
#endif
			return tfd;
		}

		std::thread t(&server_base::runThread, this, tfd);
		t.detach();
	}

	if (waitForClose) {
		while (numRunningThreads > 0) {
			sleep(1);
		}
	}

	after_end();
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
}

int server_base::handle_flow(const int & fd) { return 0; }
void server_base::after_end() {}
void server_base::after_endThread(const int & fd, int returnCode){}