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

valgrind: clean cache-sim
	valgrind --leak-check=full --log-file="valgrind.out" --show-reachable=yes -v ./cache-sim -a daxpy -d 1000
	
cache-sim: cache-sim.o
	$(CC) $(CFLAGS) -o $@ $<
	
cache-sim.o: cache-sim.cpp cache.hpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.exe