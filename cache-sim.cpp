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

static void daxpy (const CacheConfig& config) {
	CPU cpu(config);
	std::vector<Address> a;
	std::vector<Address> b;
	std::vector<Address> c;
	int n = config.totalWords/3;
	a.reserve(n);
	b.reserve(n);
	c.reserve(n);

	for (int i=0;i<n;++i) {
		a.push_back(Address(i*sizeof(double)));
		b.push_back(Address((n+i)*sizeof(double)));
		c.push_back(Address((2*n+i)*sizeof(double)));
		cpu.StoreDouble(a[i], static_cast<double>(i));
		cpu.StoreDouble(b[i], static_cast<double>(i)*2.);
		cpu.StoreDouble(c[i], 0.);
	}

	double r0 = 3.;
	double r1, r2, r3, r4;
	for (int i=0; i<n; ++i) {
		r1 = cpu.LoadDouble(a[i]);
		r2 = cpu.MultDouble(r0, r1);
		r3 = cpu.LoadDouble(b[i]);
		r4 = cpu.AddDouble(r2, r3);
		cpu.StoreDouble(c[i], r4);
	}

	for (int i=0; i<n; ++i) {
		r1 = cpu.LoadDouble(a[i]);
		r2 = cpu.MultDouble(r0, r1);
		r3 = cpu.LoadDouble(b[i]);
		r4 = cpu.AddDouble(r2, r3);
		cpu.StoreDouble(c[i], r4);
	}

	for (int i=0; i<n; ++i) {
		std::cout << cpu.LoadDouble(c[i]) << std::endl;
	}
}

int main (int argc, char ** argv) {
	CacheConfig c;
	BuildConfiguration(c, argc, argv);
	daxpy(c);

//	std::cout << std::bitset<ADDRLEN>(108504) << std::endl;
//	std::cout << std::bitset<ADDRLEN>(a.GetCacheFullIndex()) << std::endl;
//	std::cout << std::bitset<ADDRLEN>(a.GetCacheBlock()) << std::endl;
//	std::cout << std::bitset<ADDRLEN>(a.GetWord()) << std::endl;
//	std::cout << "cache-sim terminating\n";
	std::cout << "cache-sim terminating\n";
	return EXIT_SUCCESS;
}
