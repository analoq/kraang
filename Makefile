testing: test
	./test

debug: test
	lldb test -- -b

test: osx/test.cpp Buffer.hpp
	g++ -g -std=c++11 -o test osx/test.cpp
