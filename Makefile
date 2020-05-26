testing: test
	./test

debug: test
	lldb test -- -b

test: test.cpp Buffer.hpp
	g++ -g -std=c++11 -o test test.cpp
