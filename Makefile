CC=g++
DEBUG=-ggdb -pedantic -std=c++11 -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused
OPT=-pedantic -std=c++11 -O3 -Wall -Werror

ifneq (,$(filter $(MAKECMDGOALS),debug valgrind))
CFLAGS=$(DEBUG)
else
CFLAGS=$(OPT)
endif

.PHONY: all

all: clean cache-sim

debug: all

test: clean cache-sim
	@echo =================== TEST 1 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r LRU -d 10
	@echo =================== TEST 2 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r LRU -d 1000
	@echo =================== TEST 3 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r LRU -d 100000
	@echo =================== TEST 4 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r FIFO -d 10
	@echo =================== TEST 5 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r FIFO -d 1000
	@echo =================== TEST 6 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r FIFO -d 100000
	@echo =================== TEST 7 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r random -d 10
	@echo =================== TEST 8 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r random -d 1000
	@echo =================== TEST 9 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a daxpy -r random -d 100000
	@echo =================== TEST 10 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm -r FIFO -d 3
	@echo =================== TEST 11 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm -r FIFO -d 100
#	@echo =================== TEST 12 ===================
#	./cache-sim -t -c 65536 -b 64 -n 4 -a mxm -r FIFO -d 480
	@echo =================== TEST 13 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm -r LRU -d 3
	@echo =================== TEST 14 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm -r LRU -d 100
#	@echo =================== TEST 15 ===================
#	./cache-sim -t -c 65536 -b 64 -n 4 -a mxm -r LRU -d 480
	@echo =================== TEST 16 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm -r random -d 3
	@echo =================== TEST 17 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm -r random -d 100
#	@echo =================== TEST 18 ===================
#	./cache-sim -t -c 65536 -b 64 -n 4 -a mxm -r random -d 480
	@echo =================== TEST 19 ===================
	./cache-sim -t -c 512 -b 32 -n 4 -a mxm_blocking -r FIFO -d 9 -f 3
	@echo =================== TEST 20 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm_blocking -r FIFO -d 100 -f 10
	@echo =================== TEST 21 ===================
	./cache-sim -t -c 65536 -b 64 -n 4 -a mxm_blocking -r FIFO -d 400 -f 20
	@echo =================== TEST 22 ===================
	./cache-sim -t -c 512 -b 32 -n 4 -a mxm_blocking -r LRU -d 9 -f 3
	@echo =================== TEST 23 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm_blocking -r LRU -d 100 -f 10
	@echo =================== TEST 24 ===================
	./cache-sim -t -c 65536 -b 64 -n 4 -a mxm_blocking -r LRU -d 400 -f 20
	@echo =================== TEST 25 ===================
	./cache-sim -t -c 512 -b 32 -n 4 -a mxm_blocking -r random -d 9 -f 3
	@echo =================== TEST 26 ===================
	./cache-sim -t -c 4096 -b 32 -n 4 -a mxm_blocking -r random -d 100 -f 10
	@echo =================== TEST 27 ===================
	./cache-sim -t -c 65536 -b 64 -n 4 -a mxm_blocking -r random -d 400 -f 20
	@echo =================== ALL TESTS PASS ===================
	
valgrind: clean cache-sim
	valgrind --leak-check=full --log-file="valgrind.out" --show-reachable=yes -v ./cache-sim -c 4096 -b 32 -n 4 -a mxm -p -r random -d 3
	
cache-sim: cache-sim.o
	$(CC) $(CFLAGS) -o $@ $<
	
cache-sim.o: cache-sim.cpp cache.hpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.exe