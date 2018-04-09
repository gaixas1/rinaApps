#include <tclap/CmdLine.h>

#include "include/test_client_base.h"

class VideoClient : public TestClientBase {
public:
	VideoClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance, const std::string DIF,
		int FlowIdent, int QosIdent, int TestDuration,
		unsigned int _packet_size, const unsigned long long _ratebps) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			FlowIdent, QosIdent, TestDuration) {

		if (_packet_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		} else {
			sdu_size = _packet_size;
		}

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns *= sdu_size * 8L;	// b/B
		interval_ns /= _ratebps;			// b/s
		interval = std::chrono::nanoseconds(interval_ns);
	}

protected:
	int sdu_size;
	std::chrono::nanoseconds interval;																														

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t =
			std::chrono::system_clock::now();

		while (t < endtime) {
			if (SendData(sdu_size) != sdu_size) return -1;
			t += interval;
			std::this_thread::sleep_until(t);
		};

		return 0;
	}
};

int main(int argc, char ** argv) {
	std::string Name, Instance;
	std::string ServerName, ServerInstance;
	std::string DIF;
	int FlowIdent, QoSIdent, TestDuration;
	unsigned int PacketSize;
	unsigned long long RateBPS;

	try {
		TCLAP::CmdLine cmd("VideoClient", ' ', "2.0");

		//Base params
        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = OnOffClient", false, "LogServer", "string");
        TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::ValueArg<std::string> sName_a("m", "sname", "Server process name, default = DropServer", false, "DropServer", "string");
		TCLAP::ValueArg<std::string> sInstance_a("j", "sinstance", "Server process instance, default = 1",false, "1", "string");
		TCLAP::ValueArg<std::string> DIF_a("d", "dif", "DIF to use, empty for any DIF, default = \"\"",false, "", "string");

		//Client params
		TCLAP::ValueArg<int> FlowIdent_a("I", "flowid", "Unique flow identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> QoSIdent_a("q", "qosid", "QoS identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> TestDuration_a("d", "duration", "Test duration in s, default = 10",false, 10, "int");
		
		//OnOffParams
		TCLAP::ValueArg<unsigned int> PacketSize_a("S", "pSize", "Packet size in B, default 1000B", false, 1000, "unsigned int");
		TCLAP::ValueArg<float> RateMBPS_a("M", "Mbps", "Rate in Mbps, default = 10.0",false, 10.0f, "float");


		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(sName_a);
		cmd.add(sInstance_a);
		cmd.add(DIF_a);

		cmd.add(FlowIdent_a);
		cmd.add(QoSIdent_a);
		cmd.add(TestDuration_a);

		cmd.add(PacketSize_a);
		cmd.add(RateMBPS_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		ServerName = sName_a.getValue();
		ServerInstance = sInstance_a.getValue();
		DIF = DIF_a.getValue();

		FlowIdent = FlowIdent_a.getValue();
		QoSIdent = QoSIdent_a.getValue();
		TestDuration = TestDuration_a.getValue();

		PacketSize = PacketSize_a.getValue();
		float RateMBPS = RateMBPS_a.getValue();
		RateBPS = 1000000; // 1Mbps
		RateBPS *= RateMBPS;

	}
	catch (TCLAP::ArgException &e) {
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}


	VideoClient App(Name, Instance, ServerName, ServerInstance, DIF,
		FlowIdent, QoSIdent, TestDuration,
		PacketSize, RateBPS);
	return App.Run();
}