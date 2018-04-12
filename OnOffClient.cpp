#include <tclap/CmdLine.h>
#include <random>

#include "include/test_client_base.h"

class OnOffClient : public TestClientBase {
public:
	OnOffClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance, const std::string DIF,
		int FlowIdent, int QosIdent, int TestDuration,
		unsigned int _packet_size, const unsigned long long _ratebps, const int _avg_ms_on, const int _avg_ms_off) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			FlowIdent, QosIdent, TestDuration) {

		if (_packet_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		} else {
			sdu_size = _packet_size;
		}

		avg_ms_on = _avg_ms_on > 1 ? _avg_ms_on : 1;
		avg_ms_off = _avg_ms_off > 1 ? _avg_ms_off : 1;

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns *= sdu_size * 8L;	// b/B
		interval_ns /= _ratebps;			// b/s
		interval = std::chrono::nanoseconds(interval_ns);
	}

protected:
	int sdu_size;
	int avg_ms_on, avg_ms_off;
	std::chrono::nanoseconds interval;																														

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t = 
			std::chrono::system_clock::now();

		std::default_random_engine generator(t.time_since_epoch().count());
		std::exponential_distribution<double> on_distribution(1.0 / avg_ms_on);
		std::exponential_distribution<double> off_distribution(1.0 / avg_ms_off);

		bool startOff = (rand() % (avg_ms_on + avg_ms_off) <= avg_ms_off);
		long int  phase_duration;
		if (startOff) {
			phase_duration = (long int)off_distribution(generator);
			t += std::chrono::milliseconds(phase_duration);
			std::this_thread::sleep_until(t);
		}

		int i = 1;
		while (t < Endtime) {
			phase_duration = (long int)on_distribution(generator);
			auto change = t + std::chrono::milliseconds(phase_duration);
			if (change > Endtime) {
				change = Endtime;
			}

			long int sent = 0;
			while (t < change) {
				if (SendData(sdu_size) != sdu_size) return -1;
				sent++;
				t += interval;
				std::this_thread::sleep_until(t);
			}
			phase_duration = (long int)off_distribution(generator);
			t += std::chrono::milliseconds(phase_duration);
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
	int AVG_ms_ON, AVG_ms_OFF;
	std::string QoSFile;

	try {
		TCLAP::CmdLine cmd("OnOffClient", ' ', "2.0");

		//Base params
        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = OnOffClient", false, "OnOffClient", "string");
        TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::ValueArg<std::string> sName_a("m", "sname", "Server process name, default = DropServer", false, "DropServer", "string");
		TCLAP::ValueArg<std::string> sInstance_a("j", "sinstance", "Server process instance, default = 1",false, "1", "string");
		TCLAP::ValueArg<std::string> QoSFile_a("Q", "qos", "QoSRequirements filename, default = \"\"", false, "", "string");
		TCLAP::ValueArg<std::string> DIF_a("D", "dif", "DIF to use, empty for any DIF, default = \"\"",false, "", "string");

		//Client params
		TCLAP::ValueArg<int> FlowIdent_a("I", "flowid", "Unique flow identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> QoSIdent_a("q", "qosid", "QoS identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> TestDuration_a("d", "duration", "Test duration in s, default = 10",false, 10, "int");
		
		//OnOffParams
		TCLAP::ValueArg<unsigned int> PacketSize_a("S", "pSize", "Packet size in B, default 1000B", false, 1000, "unsigned int");
		TCLAP::ValueArg<int> AVG_ms_ON_a("O", "dOn", "Average duration of On interval in ms, default 1000 ms", false, 1000, "int");
		TCLAP::ValueArg<int> AVG_ms_OFF_a("F", "dOff", "Average duration of Off interval in ms, default 1000 ms",false, 1000, "int");
		TCLAP::ValueArg<float> RateMBPS_a("M", "Mbps", "Rate in Mbps, default = 10.0",false, 10.0f, "float");


		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(sName_a);
		cmd.add(sInstance_a);
		cmd.add(QoSFile_a);

		cmd.add(FlowIdent_a);
		cmd.add(QoSIdent_a);
		cmd.add(TestDuration_a);

		cmd.add(PacketSize_a);
		cmd.add(AVG_ms_ON_a);
		cmd.add(AVG_ms_OFF_a);
		cmd.add(RateMBPS_a);

		cmd.add(DIF_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		ServerName = sName_a.getValue();
		ServerInstance = sInstance_a.getValue();
		QoSFile = QoSFile_a.getValue();
		DIF = DIF_a.getValue();

		FlowIdent = FlowIdent_a.getValue();
		QoSIdent = QoSIdent_a.getValue();
		TestDuration = TestDuration_a.getValue();

		PacketSize = PacketSize_a.getValue();
		AVG_ms_ON = AVG_ms_ON_a.getValue();
		AVG_ms_OFF = AVG_ms_OFF_a.getValue();
		float RateMBPS = RateMBPS_a.getValue();
		RateBPS = 1000000; // 1Mbps
		RateBPS *= RateMBPS;

	}
	catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}


	OnOffClient App(Name, Instance, ServerName, ServerInstance, DIF,
		FlowIdent, QoSIdent, TestDuration,
		PacketSize, RateBPS, AVG_ms_ON, AVG_ms_OFF);
	App.ReadQoSFile(QoSFile);
	return App.Run();
}