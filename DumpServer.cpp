#include <chrono>
#include <map>

#include <tclap/CmdLine.h>

#include "include/ra_base_server.h"
#include "include/test_commons.h"

class DumpServer : public ra::BaseServer {
public:
	DumpServer(const std::string Name, const std::string Instance) :
		BaseServer(Name, Instance) {
		RegisterTimeoutMs = 1000;
		ListenTimeoutMs = -1;
	}

protected:
	int HandleFlow(const int Fd) {
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		long long InitTime;
		if (ra::ReadDataTimeout(Fd, Buffer, 1000) <= 0) {
#ifdef INFO
			std::cerr << "No data received during the first second of lifetime" << std::endl;
#endif
			return -1;
		}

		if (InitData.Flags & SDU_FLAG_INIT == 0) {
#ifdef INFO
			std::cerr << "First received packet not with INIT flag" << std::endl;
#endif
			return -1;
		}

#ifdef INFO
		if (InitData.Flags & SDU_FLAG_NAME) {
			std::cout << "Started Flow " << Fd << " -> " << (Buffer + sizeof(initSDU)) << std::endl;
		} else {
			std::cout << "Started Flow " << Fd << std::endl;
		}
#endif

		InitTime = Data.SendTime;

		if (write(Fd, Buffer, InitData.Size) != (int)InitData.Size) {
#ifdef INFO
			std::cerr << "First packet ECHO failed" << std::endl;
#endif
			return -1;
		}



		int ReadSize;
		for (;;) {
			if(ra::ReadData(Fd, Buffer) <= 0) { return -1;}

			if (Data.Flags & SDU_FLAG_FIN) {
				break;
			}
			std::cout << Fd<< " | " << Data.SeqId  << " | " << Data.Size << "B | " << (Data.SendTime - InitTime) << std::endl;  
		}

		if (write(Fd, Buffer, Data.Size) != (int) Data.Size) {
#ifdef INFO
			std::cerr << "Last packet ECHO failed" << std::endl;
#endif
			return -1;
		}
		return 0;
	}
};


int main(int argc, char ** argv) {
	std::string Name, Instance;
	std::vector<std::string> DIFs;

	try {
		TCLAP::CmdLine cmd("DumpServer", ' ', "2.0");

        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = DumpServer", false, "DumpServer", "string");
        TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::UnlabeledMultiArg<std::string> DIFs_a("difs","DIFs to use, empty for any DIF", false, "string");

		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(DIFs_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		DIFs = DIFs_a.getValue();
	}
	catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}

	DumpServer App(Name, Instance);
	for (auto DIF : DIFs) {
		App.AddToDIF(DIF);
	}

	return App.Run();
}