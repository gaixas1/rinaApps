#include <tclap/CmdLine.h>
#include <random>
#include <queue>

#include "include/test_client_base.h"

class Exponential : public TestClientBase {
public:
	Exponential(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance, const std::string DIF,
		int FlowIdent, int QosIdent, int TestDuration,
		unsigned int packet_size, const unsigned long long avg_flow_rate_bps) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			FlowIdent, QosIdent, TestDuration) {

		sdu_size = packet_size;
		if (sdu_size < 0 || sdu_size < sizeof(dataSDU)) {
			sdu_size = sizeof(dataSDU);
		} else if (sdu_size > BUFF_SIZE) {
			sdu_size = BUFF_SIZE;
		}

		avg_iat = 8000000000.0; //ns
		avg_iat *= sdu_size;
		avg_iat /= avg_flow_rate_bps;
	}

protected:
	double avg_iat;		
	int sdu_size;																		

	int RunFlow() {
		std::chrono::time_point<std::chrono::system_clock> t = 
			std::chrono::system_clock::now();

		std::default_random_engine generator(t.time_since_epoch().count());
		std::exponential_distribution<double> iat_distribution(1.0 / avg_iat); //nanoseconds

		auto NextCreate = t;

		while (t < Endtime) {
			while(NextCreate <= t){
				if (SendData(sdu_size) != sdu_size) return -1;
				NextCreate += std::chrono::nanoseconds((long long int)iat_distribution(generator));
			}
			if(NextCreate > Endtime){
				break;
			}
			std::this_thread::sleep_until(t);
			t = std::chrono::system_clock::now();
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
	std::string QoSFile;

	try {
		TCLAP::CmdLine cmd("Exponential", ' ', "2.0");

		//Base params
        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = Exponential", false, "Exponential", "string");
        TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::ValueArg<std::string> sName_a("m", "sname", "Server process name, default = DropServer", false, "DropServer", "string");
		TCLAP::ValueArg<std::string> sInstance_a("j", "sinstance", "Server process instance, default = 1",false, "1", "string");
		TCLAP::ValueArg<std::string> QoSFile_a("Q", "qos", "QoSRequirements filename, default = \"\"", false, "", "string");
		TCLAP::ValueArg<std::string> DIF_a("D", "dif", "DIF to use, empty for any DIF, default = \"\"",false, "", "string");

		//Client params
		TCLAP::ValueArg<int> FlowIdent_a("I", "flowid", "Unique flow identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> QoSIdent_a("q", "qosid", "QoS identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> TestDuration_a("d", "duration", "Test duration in s, default = 10",false, 10, "int");
		
		//PoissonParams
		TCLAP::ValueArg<unsigned int> PacketSize_a("S", "pSize", "Packet size in B, default 1000B", false, 1000, "unsigned int");
		TCLAP::ValueArg<float> RateMBPS_a("M", "Mbps", "Average Rate in Mbps, default = 10.0",false, 10.0f, "float");

		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(sName_a);
		cmd.add(sInstance_a);
		cmd.add(QoSFile_a);

		cmd.add(FlowIdent_a);
		cmd.add(QoSIdent_a);
		cmd.add(TestDuration_a);

		cmd.add(PacketSize_a);
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
		float RateMBPS = RateMBPS_a.getValue();
		RateBPS = 1000000; // 1Mbps
		RateBPS *= RateMBPS;

	}
	catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}


	Exponential App(Name, Instance, ServerName, ServerInstance, DIF,
		FlowIdent, QoSIdent, TestDuration,
		PacketSize, RateBPS);
	App.ReadQoSFile(QoSFile);
	return App.Run();
}
