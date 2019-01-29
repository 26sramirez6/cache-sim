#include <sys/time.h>
#include <unordered_map>
#include <string.h>
#include "cache.hpp"

//typedef unsigned long long timestamp_t;

//static timestamp_t get_timestamp() {
//	struct timeval now;
//	gettimeofday(&now, NULL);
//	return now.tv_usec + static_cast<timestamp_t>(now.tv_sec * 1e6);
//}


static void BuildConfiguration(CacheConfig& c, int argc, char ** argv) {
	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "-p")) {
			c.logging = true;
		} else if (!strcmp(argv[i], "-c")) {
			c.cacheSize = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-n")) {
			c.nWay = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-b")) {
			c.blockSize = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-d")) {
			c.matDims = atoi(argv[i+1]);
		} else if (!strcmp(argv[i],"-r")) {
			c.policy = std::string(argv[i+1]);
		} else if (!strcmp(argv[i],"-a")) {
			c.algo = std::string(argv[i+1]);
		}
	}
	c.ComputeRAMStats();
}

int main (int argc, char ** argv) {
	CacheConfig c;
	BuildConfiguration(c, argc, argv);
	CPU cpu(c);
	Address a(12345);
	cpu.LoadDouble(a);
	std::cout << "cache-sim terminating\n";
	return EXIT_SUCCESS;
}
