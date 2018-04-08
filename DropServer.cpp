#include <tclap/CmdLine.h>
#include "include/ra_base_server.h"
#include "include/test_commons.h"

class DropServer : public ra::BaseServer {
public:
	DropServer(const std::string Name, const std::string Instance) :
		BaseServer(Name, Instance) {
		RegisterTimeoutMs = 1000;
		ListenTimeoutMs = -1;
	}

protected:
	int HandleFlow(const int Fd) {
		union {
			char Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		if (ra::ReadDataTimeout(Fd, Buffer, 1000) <= 0) {
			std::cerr << "No data received during the first second of lifetime" << std::endl;
			return -1;
		}

		if (Data.Flags & SDU_FLAG_INIT == 0) {
			std::cerr << "First received packet not with INIT flag" << std::endl;
			return -1;
		}

		if (Data.Flags & SDU_FLAG_NAME) {
			cout << "Started Flow " << Fd << " -> " << (Buffer+sizeof(initSDU)) << endl;
		} else {
			cout << "Started Flow " << Fd << endl;
		}

		int ReadSize;
		for (;;) {
			ReadSize = ra::ReadDataTimeout(Fd, Buffer, 1000);
			if (ReadSize < 0) {
				return -1;
			}
			if (Data.Flags & SDU_FLAG_FIN) {
				if (Data.Flags & SDU_FLAG_NAME) {
					cout << "Ended Flow " << Fd << " -> " << (Buffer + sizeof(dataSDU)) << endl;
				} else {
					cout << "Ended Flow " << Fd << endl;
				}
				return 0;
			}
		}
	}
};


int main(int argc, char ** argv) {
	std::string Name, Instance;
	std::vector<std::string> DIFs;

	try {
		CmdLine cmd("DropServer", ' ', PACKAGE_VERSION);

		ValueArg<string> Name_a("n", "name", "",
			"Application process name, default = DropServer.",
			false, "DropServer", "string");
		ValueArg<string> Instance_a("i", "instance", "",
			"Application process instance, default = 1.",
			false, "1", "string");
		UnlabeledMultiArg<string> DIFs_a( "difs",
			"DIFs to use, empty for any DIF.",
			false, "string");

		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(DIFs_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		DIFs = DIFs_a.getValue();
	} catch (ArgException &e) {
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}

	DropServer App(Name, Instance);
	for (auto DIF : DIFs) {
		App.AddToDIF(DIF);
	}

	return App.Run();
}