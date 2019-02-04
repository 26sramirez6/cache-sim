#include <sys/time.h>
#include <unordered_map>
#include <string.h>
#include "cache.hpp"

static void BuildConfiguration(CacheConfig& c, int argc, char ** argv) {
	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "-p")) {
			c.printSolution = true;
		} else if (!strcmp(argv[i], "-c")) {
			c.cacheSize = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-n")) {
			c.nWay = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-b")) {
			c.blockSize = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-d")) {
			c.matDims = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "-f")) {
			c.blockFactor = atoi(argv[i+1]);
		} else if (!strcmp(argv[i],"-r")) {
			c.SetPolicy(argv[i+1]);
		} else if (!strcmp(argv[i],"-a")) {
			c.SetAlgo(argv[i+1]);
		} else if (!strcmp(argv[i],"-t")) {
			c.runTests = true;
		}
	}
	c.ComputeStats();
}

static void do_block (const CacheConfig& config, CPU& cpu,
	std::vector<Address>& a, std::vector<Address>& b,
	std::vector<Address>& c, uint32_t si, uint32_t sj, uint32_t sk) {

	for (uint32_t i=si; i<si+config.blockFactor; ++i) {
		for (uint32_t j=sj; j<sj+config.blockFactor; ++j) {
			double cij = cpu.LoadDouble(c[i+j*config.matDims]);
			for (uint32_t k=sk; k<sk+config.blockFactor; ++k) {
				double r1 = cpu.LoadDouble(a[i+k*config.matDims]);
				double r2 = cpu.LoadDouble(b[k+j*config.matDims]);
				cij += cpu.MultDouble(r1, r2);
			}
			cpu.StoreDouble(c[i+j*config.matDims], cij);
		}
	}
}

static void mxm_blocking (const CacheConfig& config) {
	CPU cpu(config);
	std::vector<Address> a;
	std::vector<Address> b;
	std::vector<Address> c;
	uint32_t n = config.totalWords/3;
	a.reserve(n);
	b.reserve(n);
	c.reserve(n);
	for (uint32_t i=0;i<n;++i) {
		a.push_back(Address(i*sizeof(double)));
		b.push_back(Address((n+i)*sizeof(double)));
		c.push_back(Address(((2*n)+i)*sizeof(double)));
		cpu.StoreDouble(a[i], static_cast<double>(i));
		cpu.StoreDouble(b[i], static_cast<double>(i)*2.);
		cpu.StoreDouble(c[i], 0.);
	}

	std::cout << config.blockFactor << std::endl;
	for (uint32_t sj=0; sj<config.matDims; sj+=config.blockFactor) {
		for (uint32_t si=0; si<config.matDims; si+=config.blockFactor) {
			for (uint32_t sk=0; sk<config.matDims; sk+=config.blockFactor) {
				do_block(config, cpu, a, b, c, si, sj, sk);
			}
		}
	}

	if (config.runTests) {
		for (uint32_t i=0;i<config.matDims;++i) {
			for (uint32_t j=0;j<config.matDims;++j) {
				double r4 = 0;
				for (uint32_t k=0;k<config.matDims;++k) {
					double r1 = cpu.LoadDouble(a[(i*config.matDims) + k]);
					double r2 = cpu.LoadDouble(b[j + (k*config.matDims)]);
					r4 += cpu.MultDouble(r1, r2);
				}
				assert(cpu.LoadDouble(c[i*config.matDims + j])==r4);
			}
		}
	}
	cpu.PrintStats();
	if (config.printSolution) {
		for (uint32_t i=0;i<config.matDims;++i) {
			putchar('|');
			for(uint32_t j=0; j<config.matDims; j++)
			{
				double val = cpu.LoadDouble(c[i*config.matDims + j]);
				putchar(' ');
				printf(" %.2f ", val);
				if (j==config.matDims-1 && val>=0)
					putchar(' ');
			}
			putchar('|');
			putchar('\n');
		}
	}
}


static void mxm (const CacheConfig& config) {
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
		c.push_back(Address(((2*n)+i)*sizeof(double)));
		cpu.StoreDouble(a[i], static_cast<double>(i));
		cpu.StoreDouble(b[i], static_cast<double>(i)*2.);
		cpu.StoreDouble(c[i], 0.);
	}

	for (uint32_t i=0;i<config.matDims;++i) {
		for (uint32_t j=0;j<config.matDims;++j) {
			double r4 = 0;
			for (uint32_t k=0;k<config.matDims;++k) {
				double r1 = cpu.LoadDouble(a[(i*config.matDims)+k]);
				double r2 = cpu.LoadDouble(b[j + (k*config.matDims)]);
				r4 += cpu.MultDouble(r1, r2);
			}
			cpu.StoreDouble(c[i*config.matDims + j], r4);
		}
	}

	if (config.runTests) {
		for (uint32_t i=0;i<config.matDims;++i) {
			for (uint32_t j=0;j<config.matDims;++j) {
				double r4 = 0;
				for (uint32_t k=0;k<config.matDims;++k) {
					double r1 = cpu.LoadDouble(a[(i*config.matDims)+k]);
					double r2 = cpu.LoadDouble(b[j + (k*config.matDims)]);
					r4 += cpu.MultDouble(r1, r2);
				}
				assert(cpu.LoadDouble(c[i*config.matDims + j])==r4);
			}
		}
	}

	cpu.PrintStats();

	if (config.printSolution) {
		for (uint32_t i=0;i<config.matDims;++i) {
			putchar('|');
			for(uint32_t j=0; j<config.matDims; j++)
			{
				double val = cpu.LoadDouble(c[i*config.matDims + j]);
				putchar(' ');
				printf(" %.2f ", val);
				if (j==config.matDims-1 && val>=0)
					putchar(' ');
			}
			putchar('|');
			putchar('\n');
		}
	}

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
		c.push_back(Address(((2*n)+i)*sizeof(double)));
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

	if (config.runTests) {
		for (int i=0; i<n; ++i) {
			assert(cpu.LoadDouble(c[i])==(cpu.LoadDouble(a[i])*r0 + cpu.LoadDouble(b[i])));
		}
	}
	cpu.PrintStats();

	if (config.printSolution) {
		putchar('[');
		for (int i=0; i<n; ++i) {
			std::cout << cpu.LoadDouble(c[i]) << ", ";
		}
		putchar(']');
		putchar('\n');
	}
}

int main (int argc, char ** argv) {
	CacheConfig c;
	BuildConfiguration(c, argc, argv);
	if (c.algo==c.daxpy) {
		daxpy(c);
	} else if (c.algo==c.mxm) {
		mxm(c);
	} else if (c.algo==c.mxm_blocking) {
		mxm_blocking(c);
	}
	std::cout << "cache-sim terminating\n";
	return EXIT_SUCCESS;
}
