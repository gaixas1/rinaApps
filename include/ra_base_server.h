#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "ra_commons.h"

namespace ra {
	class BaseServer {
	public:
		BaseServer(const std::string Name, const std::string Instance);
		void AddToDIF(std::string DIF);

		int Run();

	protected:
		int RegisterTimeoutMs;
		int ListenTimeoutMs;

		std::string MyName;
		std::vector<std::string> DIFs;
		bool CloseOnNextLoop;
		int  NumRunningThreads;
		bool WaitForClose;

		virtual int HandleFlow(const int Fd) { return 0; }
		virtual int AfterEnd() { return 0; };
		virtual void AfterEndFlow(const int & Fd, const int ReturnCode) {};

	private:
		std::mutex Mtx;

		void RunThread(const int & Fd);
	};

	BaseServer::BaseServer(const std::string Name, const std::string Instance) {
		MyName = Name + "|" + Instance;
		RegisterTimeoutMs = -1;
		ListenTimeoutMs = -1;
		WaitForClose = false;
	}
	void BaseServer::AddToDIF(const std::string DIF) {
		if (std::find(DIFs.begin(), DIFs.end(), DIF) == DIFs.end()) {
			DIFs.push_back(DIF);
		}
	}

	int BaseServer::Run() {
#ifdef INFO
		std::cout << "BaseServer Run()"<< std::endl;
#endif
		int Cfd = -1;

		if (DIFs.empty()) {
			if (!RegisterApp(Cfd, MyName.c_str(), RegisterTimeoutMs) ) {
#ifdef INFO
				std::cout << "Cfd = " << Cfd << " | Registered into default DIF failed" << std::endl;
#endif
				return -1;
			} 
#ifdef INFO
			else {
				std::cout << "Cfd = " << Cfd << " | Registered into default DIF" << std::endl;
			}
#endif
		} else {
			for (auto DIF : DIFs) {
				if (!RegisterApp(Cfd, MyName.c_str(), RegisterTimeoutMs, DIF.c_str() ) ) {
#ifdef INFO
					std::cout << "Cfd = " << Cfd << " | Registered into DIF " << DIF << " failed" << std::endl;
#endif
					return -1;
				}
#ifdef INFO
				else {
					std::cout << "Cfd = " << Cfd << " | Registered into DIF " << DIF << std::endl;
				}
#endif
			}
		}

		CloseOnNextLoop = false;
		NumRunningThreads = 0;
#ifdef INFO
		std::cout << "Start listening loop"<< std::endl;
#endif
		for (;;) {
			if (CloseOnNextLoop) {
#ifdef INFO
				std::cout << "Close listening loop"<< std::endl;
#endif
				break;
			}
			int Fd = ListenFlow(Cfd, ListenTimeoutMs, NULL, NULL);
			if (Fd == 0) {
				continue;
			}
			if (Fd < 0) {
#ifdef DEBUG
				std::cout << "Bad Fd from ListenFlow : "<< Fd << std::endl;
#endif
				break;
			}

#ifdef DEBUG
			std::cout << "Start new flow : "<< Fd << std::endl;
#endif
			std::thread t(&BaseServer::RunThread, this, Fd);
			t.detach();
		}

		if (WaitForClose) {
#ifdef INFO
			std::cout << "Wait for running flows"<< std::endl;
#endif
			while (NumRunningThreads > 0) {
				sleep(1);
			}
		}
#ifdef INFO
			std::cout << "Close server"<< std::endl;
#endif
		return AfterEnd();
	}

	void  BaseServer::RunThread(const int & Fd) {
		Mtx.lock();
		NumRunningThreads++;
		Mtx.unlock();

		int ReturnCode = HandleFlow(Fd);

		Mtx.lock();
		NumRunningThreads--;
		Mtx.unlock();

		AfterEndFlow(Fd, ReturnCode);

		close(Fd);
	}
}
