#pragma once


#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "ra_commons.h"

namespace ra {
	class BaseClient {
	public:
		BaseClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance, const std::string DIF);

		int Run();

		bool ReadQoSFile(const std::string QoSFile);

	protected:
		int AllocTimeoutMs;

		std::string MyName;
		std::string DstName;
		std::string DIFName;
		struct rina_flow_spec FlowSpec;

		virtual int HandleFlow(const int Fd) { return 0; }
		virtual int AllocFailed(const int ReturnCode) { return -1; };
		virtual int AfterEndFlow(const int ReturnCode) { return ReturnCode; };
	};

	BaseClient::BaseClient(const std::string Name, const std::string Instance, const std::string ServerName, const std::string ServerInstance, const std::string DIF) {
		MyName = Name + "|" + Instance;
		DstName = ServerName + "|" + ServerInstance;
		rina_flow_spec_unreliable(&FlowSpec);

		AllocTimeoutMs = -1;
	}

	int BaseClient::Run() {
#ifdef INFO
		std::cout << "BaseClient Run()"<< std::endl;
#endif
		int Fd = AllocFlow(
			MyName.c_str(),
			DstName.c_str(),
			&FlowSpec,
			AllocTimeoutMs,
			(DIFName == "" ? NULL : DIFName.c_str())
		);

		if (Fd <= 0) {
#ifdef INFO
		std::cout << "Failure allocating flow"<< std::endl;
#endif
			return AllocFailed(Fd);
		}
		
#ifdef INFO
		std::cout << "AllocFlow() success : " << Fd << " | start flow"<< std::endl;
#endif

		int ReturnCode = HandleFlow(Fd);
		close(Fd);
		return AfterEndFlow(ReturnCode);
	}

	bool BaseClient::ReadQoSFile(const std::string QoSFile) {
		if (QoSFile != "") {
			return false;
		}
		return ParseQoSRequirementsFile(&FlowSpec, QoSFile.c_str());
	}

}
