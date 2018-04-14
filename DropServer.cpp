#include <tclap/CmdLine.h>
#include "include/ra_base_server.h"
#include "include/test_commons.h"

class DropServer : public ra::BaseServer {
public:
	DropServer(const std::string Name, const std::string Instance) :
		BaseServer(Name, Instance) {
		RegisterTimeoutMs = 5000;
		ListenTimeoutMs = -1;
	}

protected:
	int HandleFlow(const int Fd) {
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		if (ra::ReadDataTimeout(Fd, Buffer, 1000) <= 0) {
#ifdef INFO
			std::cerr << "No data received during the first second of lifetime" << std::endl;
#endif
			return -1;
		}

		if (Data.Flags & SDU_FLAG_INIT == 0) {
#ifdef INFO
			std::cerr << "First received packet not with INIT flag" << std::endl;
#endif
			return -1;
		}

#ifdef INFO
		if (Data.Flags & SDU_FLAG_NAME) {
			std::cout << "Started Flow " << Fd << " -> " << (Buffer+sizeof(initSDU)) << std::endl;
		} else {
			std::cout << "Started Flow " << Fd << std::endl;
		}
#endif

		if (write(Fd, Buffer, InitData.Size) != (int)InitData.Size) {
#ifdef INFO
			std::cerr << "First packet ECHO failed" << std::endl;
#endif
			return -1;
		}

		int ReadSize;
		for (;;) {
			ReadSize = ra::ReadData(Fd, Buffer);
			if (ReadSize <= 0) {
				return -1;
			}
			if (Data.Flags & SDU_FLAG_FIN) {
#ifdef INFO
				if (Data.Flags & SDU_FLAG_NAME) {
					std::cout << "Ended Flow " << Fd << " -> " << (Buffer + sizeof(dataSDU)) << std::endl;
				} else {
					std::cout << "Ended Flow " << Fd << std::endl;
				}
#endif
				break;
			}
		}

		if (write(Fd, Buffer, Data.Size) != (int) Data.Size) {
#ifdef INFO
			std::cerr << "Last packet ECHO failed" << std::endl;
#endif
			return -1;
		}
	}
};


int main(int argc, char ** argv) {
	std::string Name, Instance;
	std::vector<std::string> DIFs;

	try {
		TCLAP::CmdLine cmd("DropServer", ' ', "2.0");

        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = DropServer", false, "DropServer", "string");
        TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::UnlabeledMultiArg<std::string> DIFs_a("difs","DIFs to use, empty for any DIF", false, "string");
		
		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(DIFs_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		DIFs = DIFs_a.getValue();
	} catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}

	DropServer App(Name, Instance);
	for (auto DIF : DIFs) {
		App.AddToDIF(DIF);
	}

	return App.Run();
}
