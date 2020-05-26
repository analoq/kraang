debug: test
	#lldb -o run test -- -b
	./test

test: test.cpp Buffer.hpp
	g++ -g -std=c++11 -o test test.cpp
