#include <algorithm>
#include <chrono>
#include <list>
#include <thread>

struct SingleRunResult
{
	typedef std::chrono::nanoseconds::rep Rep;

	Rep connectionDelay;
	Rep sendDelay;
	Rep recvDelay;
};

typedef std::list<SingleRunResult> TestResults;

struct SingleTester
{
	void operator()()
	{
		auto now = []() { return std::chrono::high_resolution_clock::now(); };

		int fd;

		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			std::cerr << "socket error" << std::endl;
			return;
		}

		auto started = now();
		// connect
		auto connected = now();

		auto startedSending = now();
		// send buffer
		auto finishedSending = now();

		auto startedReceiving = now();
		// receive buffer
		auto finishedReceiving = now();

		// compare in & out buffers

		// disconnect

		// process results
		SingleRunResult result;

		result.connectionDelay = std::chrono::duration_cast<std::chrono::nanoseconds>(connected - started).count();
		result.sendDelay = std::chrono::duration_cast<std::chrono::nanoseconds>(finishedSending - startedSending).count();
		result.recvDelay = std::chrono::duration_cast<std::chrono::nanoseconds>(finishedReceiving - startedReceiving).count();

		results.push_back(result);
	}

	TestResults results;
};

int main(int argc, char * argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <number of threads>" << std::endl;

		return -1;
	}

	int threadsNumber = std::atoi(argv[1]);

	typedef std::shared_ptr<SingleTester> SingleTesterPtr;

// катиться вниз тоже весело. Какая разница - разбиться на мотоцикле на дороге, на сноуборде на черном склоне или бухая и ширяясь?
// конец все равно один у всех

	std::list<std::thread> threads;

	std::for_each(threads.begin(), threads.end(), [](std::thread & t) { t.join(); });

	return 0;
}

