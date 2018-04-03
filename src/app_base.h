#pragma once
#include <cstdint>
#include <string>
#include <mutex>

//#include "../../api.h"
#include <rina/api.h>

typedef uint_8	byte_t;
typedef uint_8	datatype_t;

#pragma pack(push, 1)
typedef struct {
	size_t  size;
} mindata_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct : mindata_t {
	datatype_t type;
} typeddata_t;
#pragma pack(pop)


class app_base {
	public:
		app_base(
			const std::string & name,
			const std::string & instance,
			const std::string & dif);

		void setTimeout(const int & src, const int & usec);

	protected:
		int _timeout_sec, _timeout_usec;
		std::string _name;
		std::string _instance;
		std::string _dif;
		std::string _appl;

		int read(const int & fd, byte_t * buffer);
		int read(const int & fd, byte_t * buffer, const size_t  & readSize);
		int readtimed(const int & fd, byte_t * buffer, const int & sec, const int & usec);
		int readtimed(const int & fd, byte_t * buffer, const size_t  & readSize, const int & sec, const int & usec);
};

class client_base : public app_base {
public:
	client_base(
		const std::string & name,
		const std::string & instance,
		const std::string & servername,
		const std::string & serverinstance,
		const std::string & dif);

	int run ();

protected:
	std::string _servername;
	std::string _serverinstance;
	std::string _serverappl;
	struct rina_flow_spec _flow_spec;

	virtual int handle_flow(const int & fd) = 0;
};

class server_base : public app_base {
public:
	server_base(
		const std::string & name,
		const std::string & instance,
		const std::string & dif);

	int run ();

protected:
	virtual int handle_flow(const int & fd) = 0;

	virtual void after_end();
	virtual void after_endThread(const int & fd, int returnCode);

private:
	std::mutex mtx;
	bool close;
	bool waitForClose;
	int numRunningThreads;

	void runThread(const int & fd);
};