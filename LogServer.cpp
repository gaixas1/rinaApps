#include <chrono>
#include <map>

#include <tclap/CmdLine.h>

#include "include/ra_base_server.h"
#include "include/test_commons.h"

struct flow_log {
	flow_log() {
		count = 0; 
		data = 0;
		maxLat = 0;
		minLat = 9999999999;
		latCount = 0;
	}

	void process(long long t, dataSDU * sdu) {
		seq_id = sdu->SeqId;
		count++;
		data += sdu->Size;
		long long lat = t - sdu->SendTime;
		if (lat > maxLat) maxLat = lat;
		if (lat < minLat) minLat = lat;
		latCount += lat;
	}

	int QoSId;
	int flowId;

	int seq_id;
	long long count, data;

	long long maxLat;
	long long minLat;
	long double latCount;
};

struct qos_log {
	qos_log() {
		count = 0; 
		data = 0; 
		total = 0;
		maxLat = 0;
		minLat = 9999999999;
		latCount = 0;
	}
	void process(flow_log * f) {
		count += f->count;
		total += f->seq_id + 1;
		data += f->data;
		if (f->maxLat > maxLat) maxLat = f->maxLat;
		if (f->minLat < minLat) minLat = f->minLat;
		latCount += f->latCount;
	}

	long long count, total, data;

	long long maxLat;
	long long minLat;
	long double latCount;
};

class LogServer : public ra::BaseServer {
public:
	LogServer(const std::string Name, const std::string Instance) :
		BaseServer(Name, Instance) {
		RegisterTimeoutMs = 1000;
		ListenTimeoutMs = -1;
		Count = 0;
	}

protected:
	int Count;
	std::vector<flow_log*> flow_logs;
	std::map<int, qos_log> qos_logs;
	std::mutex Mt;

	void logger_t() {
		int fCount;
		std::chrono::seconds s5(5);

		do {
			std::this_thread::sleep_for(s5);
			Mt.lock();
			fCount = Count;
			Mt.unlock();
			std::cout << "Flows in progress " << fCount << std::endl;
		} while (fCount > 0);

		Mt.lock();
		std::cout << "Currently no flows in progress, print log:" << std::endl << std::endl;

		//Process log
		long long tCount = 0, tData = 0;

		for (auto f : flow_logs) {
			tCount += f->count;
			tData += f->data;
			qos_logs[f->QoSId].process(f);
		}

		std::cout << "Total :: " << tCount << " | " << (tData / 125000.0) << " Mb" << std::endl;

		for (flow_log * f : flow_logs) {
			long long c = f->count;
			long long t = f->seq_id;
			long double l = t - c;

			std::cout << "\t" << f->flowId << " (" << (int)f->QoSId << ") | "
				<< c << " / " << t << " (" << (l*100.0 / c) << " %) | " << f->data << " B"
				<< " ||" << (f->minLat / 1000.0)
				<< " -- " << (f->latCount / (1000.0*c))
				<< " -- " << (f->maxLat / 1000.0)
				<< std::endl;
		}

		for (auto qd : qos_logs) {
			int QoSId = qd.first;
			qos_log & q = qd.second;

			long long l = q.total - q.count;

			std::cout << "\t(" << QoSId << ") | "
				<< q.count << " | " << l << " | " << q.total << "  || "
				<< (q.minLat / 1000.0) << "  -- " << (q.latCount / (1000 * q.count))<< "  -- " << (q.maxLat / 1000.0);
			std::cout << "\t(" << QoSId << ")\t"
				<< (100.0*l / q.total) << " % || "
				<< (q.latCount / (1000 * q.count)) << " -- " << (q.maxLat / 1000.0) << std::endl;

		}

		for (flow_log * f : flow_logs) {
			delete f;
		}

		Count = 0;
		flow_logs.clear();
		Mt.unlock();
	}

	int HandleFlow(const int Fd) {
		union {
			ra::byte_t Buffer[BUFF_SIZE];
			dataSDU Data;
			initSDU InitData;
		};

		if (ra::ReadDataTimeout(Fd, Buffer, 1000) <= 0) {
			std::cerr << "No data received during the first second of lifetime" << std::endl;
			return -1;
		}

		if (InitData.Flags & SDU_FLAG_INIT == 0) {
			std::cerr << "First received packet not with INIT flag" << std::endl;
			return -1;
		}

		if (InitData.Flags & SDU_FLAG_NAME) {
			std::cout << "Started Flow " << Fd << " -> " << (Buffer + sizeof(initSDU)) << std::endl;
		} else {
			std::cout << "Started Flow " << Fd << std::endl;
		}

		if (write(Fd, Buffer, InitData.Size) != (int)InitData.Size) {
			std::cerr << "First packet ECHO failed" << std::endl;
			return -1;
		}


		flow_log * Flow = new flow_log();
		Flow->QoSId = InitData.QoSId;
		Flow->flowId = InitData.FlowId;
		Flow->seq_id = Data.SeqId;

		bool start_logger;

		Mt.lock();
		start_logger = (Count <= 0);
		Count++;
		flow_logs.push_back(Flow);
		Mt.unlock();

		if (start_logger) {
			std::thread t(&LogServer::logger_t, this);
			t.detach();
		}


		int ReadSize;
		for (;;) {
			ReadSize = ra::ReadData(Fd, Buffer);

			Mt.lock();

			if (ReadSize <= 0) {
				Count--;
				Mt.unlock();
				return -1;
			}

			if (Data.Flags & SDU_FLAG_FIN) {
				Count--;
				Mt.unlock();
				break;
			}

			Flow->process(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count(),
				&Data);
			Mt.unlock();

		}

		if (write(Fd, Buffer, Data.Size) != (int) Data.Size) {
			std::cerr << "Last packet ECHO failed" << std::endl;
			return -1;
		}
	}
};


int main(int argc, char ** argv) {
	std::string Name, Instance;
	std::vector<std::string> DIFs;

	try {
		TCLAP::CmdLine cmd("LogServer", ' ', "2.0");

        TCLAP::ValueArg<std::string> Name_a("n","name","Application process name, default = LogServer", false, "LogServer", "string");
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

	LogServer App(Name, Instance);
	for (auto DIF : DIFs) {
		App.AddToDIF(DIF);
	}

	return App.Run();
}