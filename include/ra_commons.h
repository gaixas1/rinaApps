#pragma once

#include <cstdint>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <poll.h>

#include <rina/api.h>

namespace ra {

	typedef uint8_t	byte_t;

	/*
	ReadData and ReadDataTimeout read from Fd to Buffer.
	- Return values:
		*  0 -> No data read
		* <0 -> Failure reading data
		* >0 -> Number of bytes read
	- Size specifies the amount of data to read, otherwise packet size is specified by the first bytes read.
	- Timeout variants return 0 if no data is available before the specified time. Timeout time is specified in milliseconds
	*/
	int ReadData(const int Fd, byte_t * Buffer);
	int ReadData(const int Fd, byte_t * Buffer, const size_t Size);
	int ReadDataTimeout(const int Fd, byte_t * Buffer, const int mSec = 0);
	int ReadDataTimeout(const int Fd, byte_t * Buffer, const size_t  Size, const int mSec = 0);

	/*
	WriteData and WriteDataTimeout send data from  Buffer to Fd.
	- Return values:
	*  0 -> No data writed
	* <0 -> Failure writing data
	* >0 -> Number of bytes wrote, can be less than Size.
	- Size specifies the amount of data to write.
	- Timeout variants return 0 if no data is available before the specified time. Timeout time is specified in milliseconds
	*/
	int WriteData(const int Fd, byte_t * Buffer, const size_t Size);
	int WriteDataTimeout(const int Fd, byte_t * Buffer, const size_t Size, const int mSec = 0);

	/*
	AllocFlow allocates a flow to AppName.
	- Return values:
		*  0 -> Failure creating flow
		* >0 -> File Descriptor of the created flow
	- Parameter DIFName = NULL -> system decides best DIF
	- If used, parameters mSec specifies the timeout for the call in milliseconds.
		If mSec not used or mSec < 0 :> blocking call
	*/
	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec, const char * DIFName = NULL);
	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec, const int mSec, const char * DIFName = NULL);

	/*
	Register the MyApp in DIFName
	- Return values:
	* false -> Failure creating flow
	* true	-> Success creating flow
	- Parameter Cfd saves the result of rina_open(). if Cfd < 0, rina_open() will be called and Cfd overwrited.
	- Parameter DIFName = NULL -> system decides best DIF
	- Parameters mSec specifies the timeout for the call. Timeout time is specified in milliseconds. mSec < 0 :> blocking call
	*/
	bool RegisterApp(int & Cfd, const char * MyName, const char * DIFName = NULL);
	bool RegisterApp(int & Cfd, const char * MyName, const int mSec, const char * DIFName = NULL);

	/*
	Listen for flow connection
	- Return values:
	* 0	 -> Timeout
	* >0 -> Flow Descriptor of new flow
	* <0 -> Error
	- Parameter Cfd has the result of rina_open().
	- Parameters mSec specifies the timeout for the call. Timeout time is specified in milliseconds. mSec < 0 :> blocking call
	*/
	int ListenFlow(const int Cfd, char **RemoteApp = NULL, struct rina_flow_spec * FlowSpec = NULL);
	int ListenFlow(const int Cfd, const int mSec, char **RemoteApp = NULL, struct rina_flow_spec * FlowSpec = NULL);

	/*
	Update FlowSpec with QoS requirements read from File "Filename"
	- Return values:
	* true	 -> File read and all parameters valid
	* false  -> Something failed
	- If the file is correctly read but some parameters are not valid, valid values are still updated.
	*/
	bool ParseQoSRequirementsFile(struct rina_flow_spec * FlowSpec, char * Filename);

	//Implementation
	int ReadData(const int Fd, byte_t * Buffer) {
		size_t  Rem = sizeof(size_t);
		ssize_t Ret = read(Fd, Buffer, Rem);
		if (Ret <= 0) {
#ifdef DEBUG
			if (Ret == 0) {
				std::cerr << "ReadData () failed: return 0" << std::endl;
			}
			else {
				std::cerr << "ReadData () failed: " << strerror(errno) << std::endl;
			}
#endif
			return Ret;
		}
		return ReadData(Fd, Buffer + Rem, (*(size_t *)Buffer) - Rem) + Rem;
	}

	int ReadData(const int Fd, byte_t * Buffer, const size_t Size) {
		int DataSize;
		size_t  Rem = Size;
		ssize_t Ret;
		do {
			Ret = read(Fd, Buffer, Rem);
			if (Ret <= 0) {
#ifdef DEBUG
				if (Ret == 0) {
					std::cerr << "ReadData () failed: return 0" << std::endl;
				}
				else {
					std::cerr << "ReadData () failed: " << strerror(errno) << std::endl;
				}
#endif
				return Ret;
			}
			Rem -= Ret;
			Buffer += Ret;
			DataSize += Ret;
		} while (Rem > 0);
		return DataSize;
	}

	int ReadDataTimeout(const int Fd, byte_t * Buffer, const int mSec) {
		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet <= 1) {
			return PollRet;
		}
		if (Fds.revents & POLLIN == 0) {
			return -1;
		}

		return ReadData(Fd, Buffer);
	}

	int ReadDataTimeout(const int Fd, byte_t * Buffer, const size_t  Size, const int mSec) {
		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet <= 1) {
			return PollRet;
		}
		if (Fds.revents & POLLIN == 0) {
			return -1;
		}

		return ReadData(Fd, Buffer, Size);
	}

	int WriteData(const int Fd, byte_t * Buffer, const size_t Size) {
		try {
			ssize_t Wt = write(Fd, Buffer, Size);
			return Wt;
		} catch (...) {
			return -1;
		}
	}

	int WriteDataTimeout(const int Fd, byte_t * Buffer, const size_t Size, const int mSec) {
		struct pollfd Fds = { .fd = Fd,.events = POLLOUT };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet <= 1) {
			return PollRet;
		}
		if (Fds.revents & POLLOUT == 0) {
			return -1;
		}

		return WriteData(Fd, Buffer, Size);
	}

	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec, const char * DIFName) {
		int Fd = rina_flow_alloc(DIFName, MyName, AppName, FlowSpec, 0);
#ifdef DEBUG
		if (Fd < 0) {
			std::cerr << "rina_flow_alloc () failed: " << strerror(errno) << std::endl;
		}
#endif
		return Fd;
	}

	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec, const int mSec, const char * DIFName) {
		if (mSec < 0) {
			return AllocFlow(MyName, AppName, FlowSpec, DIFName);
		}

		int Fd = rina_flow_alloc(DIFName, MyName, AppName, FlowSpec, RINA_F_NOWAIT);
#ifdef DEBUG
		if (Fd < 0) {
			std::cerr << "rina_flow_alloc () failed: " << strerror(errno) << std::endl;
		}
#endif
		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet <= 1 || Fds.revents & POLLIN == 0) {
#ifdef DEBUG
			std::cerr << "rina_flow_alloc () failed: timeout" << std::endl;
#endif
			return -1;
		}
		Fd = rina_flow_alloc_wait(Fd); 
#ifdef DEBUG
		if (Fd < 0) {
			std::cerr << "rina_flow_alloc_wait () failed: " << strerror(errno) << std::endl;
		}
#endif
		return Fd;
	}

	bool RegisterApp(int & Cfd, const char * MyName, const char * DIFName) {
		if (Cfd < 0) {
			Cfd = rina_open();
			if (Cfd < 0) {
#ifdef DEBUG
				std::cerr << "rina_open () failed: return " << Cfd << std::endl;
#endif
				return false;
			}
		}

		if (rina_register(Cfd, DIFName, MyName, 0) < 0) {
#ifdef DEBUG
			std::cerr << "rina_register () failed: " << strerror(errno) << std::endl;
#endif
			return false;
		}
		return true;
	}

	bool RegisterApp(int & Cfd, const char * MyName, const int mSec, const char * DIFName) {
		if (mSec < 0) {
			return RegisterApp(Cfd, MyName, DIFName);
		}

		if (Cfd < 0) {
			Cfd = rina_open();
			if (Cfd < 0) {
#ifdef DEBUG
				std::cerr << "rina_open () failed: return " << Cfd << std::endl;
#endif
				return false;
			}
		}

		int Fd = rina_register(Cfd, DIFName, MyName, 0);
		if (Fd < 0) {
#ifdef DEBUG
			std::cerr << "rina_register () failed: " << strerror(errno) << std::endl;
#endif
			return false;
		}

		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet <= 1 || Fds.revents & POLLIN == 0) {
#ifdef DEBUG
			std::cerr << "rina_register () failed: timeout" << std::endl;
#endif
			return false;
		}

		if (rina_register_wait(Cfd, Fd) < 0) {
#ifdef DEBUG
			std::cerr << "rina_register_wait () failed: " << strerror(errno) << std::endl;
#endif
			return false;
		}
		return true;
	}

	int ListenFlow(const int Cfd, char **RemoteApp, struct rina_flow_spec * FlowSpec) {
		int Fd = rina_flow_accept(Cfd, RemoteApp, FlowSpec, 0);
#ifdef DEBUG
		if (Fd < 0) {
			std::cerr << "rina_flow_accept () failed: " << strerror(errno) << std::endl;
		}
#endif
		return Fd;
	}

	int ListenFlow(const int Cfd, const int mSec, char **RemoteApp, struct rina_flow_spec * FlowSpec) {
		if (mSec < 0) {
			return ListenFlow(Cfd, RemoteApp, FlowSpec);
		}


		struct pollfd Fds = { .fd = Cfd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);
		if (PollRet <= 1 || Fds.revents & POLLIN == 0) {
#ifdef DEBUG
			std::cerr << "ListenFlow () failed: timeout" << std::endl;
#endif
			return 0;
		}

		int Fd = rina_flow_accept(Cfd, RemoteApp, FlowSpec, RINA_F_NORESP);
#ifdef DEBUG
		if (Fd < 0) {
			std::cerr << "rina_flow_accept () failed: " << strerror(errno) << std::endl;
		}
#endif
		Fd = rina_flow_respond(Cfd, Fd, 0);
#ifdef DEBUG
		if (Fd < 0) {
			std::cerr << "rina_flow_respond () failed: " << strerror(errno) << std::endl;
		}
#endif

		return Fd;
	}

	bool ParseQoSRequirementsFile(struct rina_flow_spec * FlowSpec, char * Filename) {
		bool ReturnValue = true;

		std::ifstream File(Filename);
		if (!File.is_open()) {
			return false;
		}

		std::string Param, Value;
		while (File >> Param >> Value) {
			if (Param != "" && Value != "") {
				if (Param == "reliable" || Param == "Reliable") {
					FlowSpec.max_sdu_gap = (Value == "true") ? 0 : -1;
				}
				else  if (Param == "orderedDelivery" || Param == "OrderedDelivery") {
					FlowSpec->in_order_delivery = (Value == "true") ? false : true;
				}
				else  if (Param == "msgBoundaries" || Param == "MsgBoundaries") {
					FlowSpec->msg_boundaries = (Value == "true") ? false : true;
				}
				else if (Param == "maxAllowableGap" || Param == "MaxAllowableGap") {
					FlowSpec->max_sdu_gap = stoi(Value);
				}
				else if (Param == "delay" || Param == "Delay") {
					FlowSpec->max_delay = stoi(Value);
				}
				else if (Param == "jitter" || Param == "Jitter") {
					FlowSpec->max_jitter = stoi(Value);
				}
				else if (Param == "averageBandwidth" || Param == "AverageBandwidth") {
					FlowSpec->avg_bandwidth = stoi(Value);
					if (FlowSpec->avg_bandwidth <= 0) {
						FlowSpec->avg_bandwidth = 1;
					}
				}
				else if (Param == "loss" || Param == "Loss") {
					FlowSpec->max_loss = stoi(Value);
				}
				else {
					ReturnValue = false;
				}
			}
		}
		File.close();

		return ReturnValue;
}
