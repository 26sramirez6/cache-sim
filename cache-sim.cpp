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
			c.SetPolicy(argv[i+1]);
		} else if (!strcmp(argv[i],"-a")) {
			c.SetAlgo(argv[i+1]);
		}
	}
	c.ComputeStats();
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
//	std::cout << "a: ";
//	for (int i=0;i<n;++i) {
//		std::cout << a[i].address_ << " : " << cpu.LoadDouble(a[i]) << ", ";
//	}
//	putchar('\n');
//
//	std::cout << "b: ";
//	for (int i=0;i<n;++i) {
//		std::cout << b[i].address_ << " : " <<  cpu.LoadDouble(b[i]) << ", ";
//	}
//	putchar('\n');
//
//	std::cout << "c: ";
//	for (int i=0;i<n;++i) {
//		std::cout << c[i].address_ << " : " <<  cpu.LoadDouble(c[i]) << ", ";
//	}
//	putchar('\n');


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

//	for (uint32_t i=0;i<config.matDims;++i) {
//		putchar('|');
//		for(uint32_t j=0; j<config.matDims; j++)
//		{
//			double val = cpu.LoadDouble(c[i*config.matDims + j]);
//			putchar(' ');
//			printf(" %.2f ", val);
//			if (j==config.matDims-1 && val>=0)
//				putchar(' ');
//		}
//		putchar('|');
//		putchar('\n');
//	}
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
//	std::cout << "a: ";
//	for (int i=0;i<n;++i) {
//		std::cout << cpu.LoadDouble(a[i]) << ", ";
//	}
//	putchar('\n');
//
//	std::cout << "b: ";
//	for (int i=0;i<n;++i) {
//		std::cout << cpu.LoadDouble(b[i]) << ", ";
//	}
//	putchar('\n');

	double r0 = 3.;
	double r1, r2, r3, r4;
	for (int i=0; i<n; ++i) {
		r1 = cpu.LoadDouble(a[i]);
		r2 = cpu.MultDouble(r0, r1);
		r3 = cpu.LoadDouble(b[i]);
		r4 = cpu.AddDouble(r2, r3);
		cpu.StoreDouble(c[i], r4);
	}

//	std::cout << "c: ";
	for (int i=0; i<n; ++i) {
//		std::cout << cpu.LoadDouble(c[i]) << " ,";
		assert(cpu.LoadDouble(c[i])==(cpu.LoadDouble(a[i])*r0 + cpu.LoadDouble(b[i])));
	}
//	putchar('\n');
	if (config.logging) cpu.PrintStats();
}

//struct B {
//	std::vector<double> data;
//	B() : data(100) { std::cout << "B constructor called" << std::endl;}
//	B(const B &other) : data(other.data) {
//		std::cout << "B copy ctor called\n";
//	}
//
//	B(B &&other) : data(std::move(other.data)) {
//		std::cout << "B move ctor called\n";
//	}
//
//	~B() {std::cout<<"B destructor called" << std::endl;}
//};
//
//struct A {
//	std::vector<B> data;
//	A() : data(100) {
//		std::cout << "Constructing A\n";
//	}
//	B TestCopy(int index) {
//		return data[index];
//	}
//};
//
//struct C {
//	B block;
//	int x;
////	C(B b) : block(std::move(b)) {
////		x = 4;
////		std::cout << "C constructor called." << std::endl;
////	}
//
//	C(B& b) : block(std::move(b)) {
//		x = 4;
//		std::cout << "C special constructor called." << std::endl;
//	}
//
//	C(const C &other): block(other.block), x(other.x) {
//		std::cout << "C copy ctor called\n";
////		block.data = other.block.data;
////		std::cout << other.block.data[0] << std::endl;
////		std::cout << block.data[0] << std::endl;
//	}
//
//	C(C &&other) : block(std::move(other.block)), x(other.x) {
//		std::cout << "C move ctr called." << std::endl;
//	}
//
//	~C() {std::cout<<"C destructor called" << std::endl;}
//};
//
//
//static void func(std::vector<C>& vec) {
//	B b;
//	b.data[0] = 88;
//
//	C c(b);
////	c.x = 19;
//	std::cout << "push back" << std::endl;
//	vec.push_back(std::move(c));
//}
//
//static void read(std::vector<C>& vec) {
//	std::cout<<vec[0].x<<std::endl;
//	std::cout<<vec[0].block.data[0]<<std::endl;
//}

int main (int argc, char ** argv) {
	CacheConfig c;
	BuildConfiguration(c, argc, argv);
	if (c.algo==c.daxpy) {
		daxpy(c);
	} else if (c.algo==c.mxm) {
		mxm(c);
	}
	std::cout << "cache-sim terminating\n";
	return EXIT_SUCCESS;
}
