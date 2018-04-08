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
		virtual int AfterEnd() { return 0 };
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
		int Cfd = -1;
		if (DIFs.empty()) {
			if (!RegisterApp(Cfd, MyName.c_str(), RegisterTimeoutMs)) {
				return -1;
			}
		} else {
			for (auto DIF : DIFs) {
				if (!RegisterApp(Cfd, MyName.c_str()), RegisterTimeoutMs, DIF.c_str()) {
					return -1;
				}
			}
		}

		CloseOnNextLoop = false;
		NumRunningThreads = 0;
		for (;;) {
			if (CloseOnNextLoop) {
				break;
			}
			int Fd = ListenFlow(Cfd, ListenTimeoutMs, NULL, NULL);
			if (Fd == 0) {
				continue;
			}
			if (Fd < 0) {
				break;
			}

			std::thread t(&server_base::RunThread, this, fd);
			t.detach();
		}

		if (WaitForClose) {
			while (NumRunningThreads > 0) {
				sleep(1);
			}
		}
		return AfterEnd();
	}

	void  BaseServer::RunThread(const int & Fd) {
		Mtx.lock();
		NumRunningThreads++;
		Mtx.unlock();

		int Return = handle_flow(Fd);

		Mtx.lock();
		NumRunningThreads--;
		Mtx.unlock();

		after_endThread(Fd, Return Code);

		close(Fd);
	}
}