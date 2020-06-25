testing: test
	./test

debug: test
	lldb test -- -b

test: osx/test.cpp Buffer.hpp Sequence.hpp MIDIFile.hpp Player.hpp Event.hpp
	rm -f *.gcda *.gcno *.gcov
	g++ -g -std=c++11 -o test osx/test.cpp

play: osx/play.cpp  Buffer.hpp Sequence.hpp MIDIFile.hpp Player.hpp Event.hpp
	g++ -g -std=c++11 -framework CoreFoundation -framework CoreMIDI -o play osx/play.cpp

record: osx/record.cpp  Buffer.hpp Sequence.hpp MIDIFile.hpp Player.hpp Event.hpp
	g++ -g -std=c++11 -framework CoreFoundation -framework CoreMIDI -lcurses -o record osx/record.cpp
