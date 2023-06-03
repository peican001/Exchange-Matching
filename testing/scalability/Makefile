all: test

test: test.cpp
	g++ -O3 -std=c++11 -pthread  -lxerces-c -o test test.cpp

.PHONY:
clean:
	rm -f test *.o *~
