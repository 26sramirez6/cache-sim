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


struct Context {
	uint32_t nWay;
	uint32_t cacheSize;
	uint32_t blockSize;
	uint32_t matDims;
	uint32_t blockFactor;
	bool logging;
	std::string policy;
	std::string algo;
	Context(): nWay(2), cacheSize(65536),
		blockSize(64), matDims(480),
		blockFactor(32), logging(false),
		policy("LRU"), algo("mxm_block") {};

	uint32_t ComputeRAMSize() const {
		uint32_t ret = 0;
		if (this->policy=="mxm_block" || this->policy=="mxm") {
			ret += this->matDims*this->matDims*sizeof(double)*3;
		} else {
			ret += this->matDims*sizeof(double)*3;
		}
		ret += (ret%blockSize);
		return ret;
	}
};

static void BuildContext(Context& c, int argc, char ** argv) {
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
}

int main (int argc, char ** argv) {
	Context c;
	BuildContext(c, argc, argv);
	CPU cpu(c.nWay, c.cacheSize, c.blockSize, c.ComputeRAMSize());
	std::cout << "success\n";
	return EXIT_SUCCESS;
}
