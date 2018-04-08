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

		if (_packet_size < MIN_PDU) {
			sdu_size = MIN_PDU;
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
		interval = nanoseconds(interval_ns);
	}

protected:
	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t = 
			std::chrono::system_clock::now();

		default_random_engine generator(t.time_since_epoch().count());
		exponential_distribution<double> on_distribution(1.0 / avg_ms_on);
		exponential_distribution<double> off_distribution(1.0 / avg_ms_off);

		bool startOff = (rand() % (avg_ms_on + avg_ms_off) <= avg_ms_off);
		long int  phase_duration;
		if (startOff) {
			phase_duration = (long int)off_distribution(generator);
			t += std::chrono::milliseconds(phase_duration);
			sleep_until(t);
		}

		int i = 1;
		while (t < endtime) {
			phase_duration = (long int)on_distribution(generator);
			change = t + std::chrono::milliseconds(phase_duration);
			if (change > endtime) {
				change = endtime;
			}

			long int sent = 0;
			while (t < change) {
				if (SendData(sdu_size) != sdu_size) return -1;
				sent++;
				t += interval;
				sleep_until(t);
			}
			phase_duration = (long int)off_distribution(generator);
			t += milliseconds(phase_duration);
			sleep_until(t);
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

	try {
		CmdLine cmd("OnOffClient", ' ', PACKAGE_VERSION);

		//Base params
		ValueArg<string> Name_a("n", "name", "",
			"Application process name, default = OnOffClient.",
			false, "OnOffClient", "string");
		ValueArg<string> Instance_a("i", "instance", "",
			"Application process instance, default = 1.",
			false, "1", "string");
		ValueArg<string> sName_a("m", "sname", "",
			"Server process name, default = DropServer.",
			false, "DropServer", "string");
		ValueArg<string> sInstance_a("j", "sinstance", "",
			"Server process instance, default = 1.",
			false, "1", "string");
		ValueArg<string> DIF_a("d", "dif", "",
			"DIF to use, empty for any DIF, default = \"\".",
			false, "", "string");

		//Client params
		ValueArg<int> FlowIdent_a("I", "flowid", "",
			"Unique flow identifier, default = 0.",
			false, 0, "int");
		ValueArg<int> QoSIdent_a("q", "qosid", "",
			"QoS identifier, default = 0.",
			false, 0, "int");
		ValueArg<int> TestDuration_a("d", "duration", "",
			"Test duration in s, default = 10.",
			false, 10, "int");
		
		//OnOffParams
		ValueArg<unsigned int> PacketSize_a("S", "pSzie",
			"Packet size in B, default 1000B",
			false, 1000, "unsigned int");
		ValueArg<int> AVG_ms_ON_a("O", "dOn",
			"Average duration of On interval in ms, default 1000 ms",
			false, 1000, "int");
		ValueArg<int> AVG_ms_OFF_a("F", "dOff",
			"Average duration of Off interval in ms, default 1000 ms",
			false, 1000, "int");
		ValueArg<float> RateMBPS_a("M", "Mbps",
			"Rate in Mbps, default = 10.0.",
			false, 10.0f, "float");


		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(sName_a);
		cmd.add(sInstance_a);
		cmd.add(DIFs_a);

		cmd.add(FlowIdent_a);
		cmd.add(QoSIdent_a);
		cmd.add(TestDuration_a);

		cmd.add(PacketSize_a);
		cmd.add(AVG_ms_ON_a);
		cmd.add(AVG_ms_OFF_a);
		cmd.add(RateMBPS_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		ServerName = sName_a.getValue();
		ServerInstance = sInstance_a.getValue();
		DIFs = DIFs_a.getValue();

		FlowIdent = FlowIdent_a.getValue();
		QoSIdent = QoSIdent_a.getValue();
		TestDuration = TestDuration_a.getValue();

		PacketSize_a = PacketSize_a.getValue();
		AVG_ms_ON = AVG_ms_ON_a.getValue();
		AVG_ms_OFF = AVG_ms_OFF_a.getValue();
		float RateMBPS = RateMBPS_a.getValue();

	}
	catch (ArgException &e) {
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}

	RateBPS = 1000000; // 1Mbps
	ratebps *= RateMBPS;

	OnOffClient App(Name, Instance, ServerName, ServerInstance, DIF,
		FlowIdent, QoSIdent, TestDuration,
		PacketSize, RateBPS, AVG_ms_ON, AVG_ms_OFF);
	return App.Run();
}