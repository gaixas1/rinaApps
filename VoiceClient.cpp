#include <tclap/CmdLine.h>
#include <stdlib.h>
#include <time.h>

#include "include/test_client_base.h"

class VoiceClient : public TestClientBase {
public:
	VoiceClient(const std::string Name, const std::string Instance, const std::string Servername, const std::string ServerInstance, const std::string DIF,
		int FlowIdent, int QosIdent, int TestDuration,
		float HZ) :
		TestClientBase(Name, Instance, Servername, ServerInstance, DIF,
			FlowIdent, QosIdent, TestDuration) {

		setON(320, 3000, 1000);
		setOFF(20, 6000, 2000);

		long long interval_ns;
		interval_ns = 1000000000L;		// ns/s
		interval_ns /= HZ;				// 1/s
		interval = std::chrono::nanoseconds(interval_ns);
	}

	void setON(unsigned int _packet_size, int min_ms, int var_ms) {
		if (_packet_size < sizeof(dataSDU)) {
			on_sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			on_sdu_size = BUFF_SIZE;
		} else {
			on_sdu_size = _packet_size;
		}

		if (min_ms <= 0) {
			min_ms = 1;
		}
		if (var_ms < 0) {
			var_ms = 0;
		}

		min_ms_on = min_ms;
		var_ms_on = var_ms;
	}

	void setOFF(unsigned int _packet_size, int min_ms, int var_ms) {
		if (_packet_size == 0) {
			off_sdu_size = 0;
		} else if (_packet_size < sizeof(dataSDU)) {
			off_sdu_size = sizeof(dataSDU);
		} else if (_packet_size > BUFF_SIZE) {
			off_sdu_size = BUFF_SIZE;
		} else {
			off_sdu_size = _packet_size;
		}

		if (min_ms <= 0) {
			min_ms = 1;
		}
		if (var_ms < 0) {
			var_ms = 0;
		}

		min_ms_off = min_ms;
		var_ms_off = var_ms;
	}

protected:
	int on_sdu_size, off_sdu_size;
	int min_ms_on, var_ms_on;
	int min_ms_off, var_ms_off;
	std::chrono::nanoseconds interval;																														

	int RunFlow() {
		unsigned int sdu_size;
		std::chrono::time_point<std::chrono::system_clock> t = 
			std::chrono::system_clock::now();
		auto change = t;

		bool isOn = (rand() % (min_ms_on + min_ms_off) <= min_ms_on);

		if (isOn) {
			sdu_size = on_sdu_size;
			change += std::chrono::milliseconds(rand() % min_ms_on);
		} else {
			sdu_size = off_sdu_size;
			change += std::chrono::milliseconds(rand() % min_ms_off);
		}

		while (t < Endtime) {
			if (t > change) {
				if (isOn) {
					isOn = false;
					sdu_size = off_sdu_size;
					change = t + std::chrono::milliseconds(min_ms_off + rand() % var_ms_off);
				} else {
					isOn = true;
					sdu_size = on_sdu_size;
					change = t + std::chrono::milliseconds(min_ms_on + rand() % var_ms_on);
				}
			}
			if (sdu_size > 0) {
				if (SendData(sdu_size) != sdu_size) return -1;
			}
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
	float HZ;
	int PacketSizeOn, AVG_ms_ON, VAR_ms_ON;
	int PacketSizeOff, AVG_ms_OFF, VAR_ms_OFF;
	
	srand(time(NULL));

	try {
		TCLAP::CmdLine cmd("VoiceClient", ' ', "2.0");

		//Base params
        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = OnOffClient", false, "LogServer", "string");
        TCLAP::ValueArg<std::string> Instance_a("i","instance","Application process instance, default = 1", false, "1", "string");
		TCLAP::ValueArg<std::string> sName_a("m", "sname", "Server process name, default = DropServer", false, "DropServer", "string");
		TCLAP::ValueArg<std::string> sInstance_a("j", "sinstance", "Server process instance, default = 1",false, "1", "string");
		TCLAP::ValueArg<std::string> DIF_a("D", "dif", "DIF to use, empty for any DIF, default = \"\"",false, "", "string");

		//Client params
		TCLAP::ValueArg<int> FlowIdent_a("I", "flowid", "Unique flow identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> QoSIdent_a("q", "qosid", "QoS identifier, default = 0",false, 0, "int");
		TCLAP::ValueArg<int> TestDuration_a("d", "duration", "Test duration in s, default = 10",false, 10, "int");
		
		//OnOffParams
		TCLAP::ValueArg<float> HZ_a("z", "hz", "Frame Rate flow, default = 50.0Hz", false, 50.0f, "float");

		TCLAP::ValueArg<int> PacketSizeOn_a("S", "oSize", "Packet size in B when ON, default 1000B", false, 320, "unsigned int");
		TCLAP::ValueArg<int> AVG_ms_ON_a("O", "dOn", "Average duration of On interval in ms, default 1000 ms", false, 3000, "int");
		TCLAP::ValueArg<int> VAR_ms_ON_a("o", "vOn", "Variation of duration of On interval in ms, default 1000 ms", false, 1000, "int");
		
		TCLAP::ValueArg<int> PacketSizeOff_a("s", "fSize", "Packet size in B when OFF, default 1000B", false, 20, "unsigned int");
		TCLAP::ValueArg<int> VAR_ms_OFF_a("F", "dOff", "Average duration of Off interval in ms, default 1000 ms", false, 6000, "int");
		TCLAP::ValueArg<int> AVG_ms_OFF_a("f", "vOff", "Variation of duration of Off interval in ms, default 1000 ms", false, 2000, "int");
		

		cmd.add(Name_a);
		cmd.add(Instance_a);
		cmd.add(sName_a);
		cmd.add(sInstance_a);
		cmd.add(DIF_a);

		cmd.add(FlowIdent_a);
		cmd.add(QoSIdent_a);
		cmd.add(TestDuration_a);

		cmd.add(HZ_a);
		cmd.add(PacketSizeOn_a);
		cmd.add(AVG_ms_ON_a);
		cmd.add(VAR_ms_ON_a);
		cmd.add(PacketSizeOff_a);
		cmd.add(AVG_ms_OFF_a);
		cmd.add(VAR_ms_OFF_a);

		cmd.parse(argc, argv);

		Name = Name_a.getValue();
		Instance = Instance_a.getValue();
		ServerName = sName_a.getValue();
		ServerInstance = sInstance_a.getValue();
		DIF = DIF_a.getValue();

		FlowIdent = FlowIdent_a.getValue();
		QoSIdent = QoSIdent_a.getValue();
		TestDuration = TestDuration_a.getValue();

		HZ = HZ_a.getValue();
		PacketSizeOn = PacketSizeOn_a.getValue();
		AVG_ms_ON = AVG_ms_ON_a.getValue();
		VAR_ms_ON = VAR_ms_ON_a.getValue();
		PacketSizeOff = PacketSizeOff_a.getValue();
		AVG_ms_OFF = AVG_ms_OFF_a.getValue();
		VAR_ms_OFF = VAR_ms_OFF_a.getValue();

	}
	catch (TCLAP::ArgException &e) {
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << "Failure reading parameters." << std::endl;
		return -1;
	}


	VoiceClient App(Name, Instance, ServerName, ServerInstance, DIF,
		FlowIdent, QoSIdent, TestDuration,
		HZ);
	App.setON(PacketSizeOn, AVG_ms_ON, VAR_ms_ON);
	App.setON(PacketSizeOff, AVG_ms_OFF, VAR_ms_OFF);
	return App.Run();
}
