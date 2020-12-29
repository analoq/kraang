#define CATCH_CONFIG_MAIN
#include <iostream>
#include "catch.hpp"
#include "../Buffer.hpp"
#include "../Event.hpp"
#include "../Sequence.hpp"
#include "../MIDIFile.hpp"
#include "../Player.hpp"
#include "CFile.hpp"

using namespace std;

class TestNode
{
public:
  char value;

  TestNode() : value('?')
  {
  }

  TestNode(int v) : value(v)
  {
  }

  bool operator <=(const TestNode &a) const
  {
    return value <= a.value;
  }

  bool operator >(const TestNode &a) const
  {
    return value > a.value;
  }

  bool operator >=(const TestNode &a) const
  {
    return value >= a.value;
  }
};

ostream& operator<<(ostream &o, const TestNode& node)
{
  o << node.value;
  return o;
}

class TestTiming
{
private:
  uint32_t microseconds;
public:
  TestTiming() : microseconds{0}
  {
  }

  uint32_t getMicroseconds() const
  {
    return microseconds;
  }
 
  void delay(uint32_t us)
  {
    microseconds += us;
  }
};


class TestMIDIPort : public MIDIPort
{
private:
  stringstream log;
  uint32_t time;
public:
  TestMIDIPort()
  {
  }

  void send(const uint8_t channel, const Event &event)
  {
    log << time << ":" << static_cast<int>(channel) << ":" << event;
  }

  void setTime(uint32_t t)
  {
    time = t;
  }

  void clear()
  {
    log.str("");
  }

  string getLog() const
  {
    return log.str();
  }
};

TEST_CASE("New buffer is empty", "[buffer]")
{
  Buffer<TestNode, 3, 1> buffer;
  REQUIRE(buffer.getHead(0) == UNDEFINED);
  REQUIRE(buffer.getTail(0) == UNDEFINED);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.dump() == "u:1:? u:2:? u:u:? ");
  REQUIRE(buffer.traverse(0) == "");
}

TEST_CASE("Insert One Track", "[buffer]")
{
  Buffer<TestNode, 5, 1> buffer;
  buffer.insert(0, TestNode('C'));
  REQUIRE(buffer.getAvailable() == 1);
  REQUIRE(buffer.dump() == "u:u:C u:2:? u:3:? u:4:? u:u:? ");
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 0);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "C");

  buffer.insert(0, TestNode('C'));
  REQUIRE(buffer.getAvailable() == 2);
  REQUIRE(buffer.dump() == "1:u:C u:0:C u:3:? u:4:? u:u:? ");
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getTail(0) == 0);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "CC");

  buffer.insert(0, TestNode('D'));
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.dump() == "1:2:C u:0:C 0:u:D u:4:? u:u:? ");
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getTail(0) == 2);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "CCD");

  buffer.insert(0, TestNode('A'));
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.dump() == "1:2:C 3:0:C 0:u:D u:1:A u:u:? ");
  REQUIRE(buffer.getHead(0) == 3);
  REQUIRE(buffer.getTail(0) == 2);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "ACCD");

  buffer.insert(0, TestNode('B'));
  REQUIRE(buffer.getAvailable() == UNDEFINED);
  REQUIRE(buffer.dump() == "1:2:C 4:0:C 0:u:D u:4:A 3:1:B ");
  REQUIRE(buffer.getHead(0) == 3);
  REQUIRE(buffer.getTail(0) == 2);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "ABCCD");
}

/*TEST_CASE("Buffer full", "[buffer]")
{
  Buffer<TestNode,4, 1> buffer;
  REQUIRE(buffer.insert(0, TestNode('A')).new_node != UNDEFINED);
  REQUIRE(buffer.insert(0, TestNode('B')).new_node != UNDEFINED);
  REQUIRE(buffer.insert(0, TestNode('C')).new_node != UNDEFINED);
  REQUIRE(buffer.insert(0, TestNode('D')).new_node != UNDEFINED);
  REQUIRE(buffer.insert(0, TestNode(5)).new_node == UNDEFINED);
}*/

TEST_CASE("Buffer clear", "[buffer]")
{
  Buffer<TestNode,4, 1> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));
  buffer.clear();
  REQUIRE(buffer.getHead(0) == UNDEFINED);
  REQUIRE(buffer.getTail(0) == UNDEFINED);
  REQUIRE(buffer.getPointer(0) ==  UNDEFINED);
  REQUIRE(buffer.dump() == "u:1:? u:2:? u:3:? u:u:? ");
  REQUIRE(buffer.traverse(0) == "");
}

TEST_CASE("Insert Two Track", "[buffer]")
{
  Buffer<TestNode, 3, 2> buffer;
  buffer.insert(0, TestNode('A'));
  buffer.insert(1, TestNode('C'));
  REQUIRE(buffer.dump() == "u:u:A u:u:C u:u:? ");
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 0);
  REQUIRE(buffer.getHead(1) == 1);
  REQUIRE(buffer.getTail(1) == 1);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.getPointer(1) == 1);
  REQUIRE(buffer.traverse(0) == "A");
  REQUIRE(buffer.traverse(1) == "C");

  buffer.insert(1, TestNode('B'));
  REQUIRE(buffer.dump() == "u:u:A 2:u:C u:1:B ");
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 0);
  REQUIRE(buffer.getHead(1) == 2);
  REQUIRE(buffer.getTail(1) == 1);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.getPointer(1) == 1);
  REQUIRE(buffer.traverse(0) == "A");
  REQUIRE(buffer.traverse(1) == "BC");
}


TEST_CASE("Buffer deletions", "[buffer]")
{
  Buffer<TestNode, 4, 1> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));
  // "u:1:A 0:2:B 1:3:C 2:u:D "  ABCD
  
  // delete from head
  buffer.setPointer(0, 0);
  buffer.remove(0);
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getTail(0) == 3);
  REQUIRE(buffer.getPointer(0) == 1);
  REQUIRE(buffer.dump() == "u:u:? u:2:B 1:3:C 2:u:D ");
  REQUIRE(buffer.traverse(0) == "BCD");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));
  // u:1:A 0:2:B 0:3:C 2:u:D 

  // delete from middle
  buffer.setPointer(0, 1);
  buffer.remove(0);
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 3);
  REQUIRE(buffer.getPointer(0) == 2);
  REQUIRE(buffer.dump() == "u:2:A u:u:? 0:3:C 2:u:D ");
  REQUIRE(buffer.traverse(0) == "ACD");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));

  // delete from end
  buffer.setPointer(0, 3);
  buffer.remove(0);
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 2);
  REQUIRE(buffer.getPointer(0) == UNDEFINED);
  REQUIRE(buffer.dump() == "u:1:A 0:2:B 1:u:C u:u:? ");
  REQUIRE(buffer.traverse(0) == "ABC");
}

TEST_CASE("Buffer deletion/insertion", "[buffer]")
{
  Buffer<TestNode, 5, 1> buffer;
  // add only element
  buffer.insert(0, TestNode('A'));
  REQUIRE(buffer.getAvailable() == 1);
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 0);
  REQUIRE(buffer.traverse(0) == "A");
  
  // delete only element
  buffer.remove(0);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead(0) == UNDEFINED);
  REQUIRE(buffer.getTail(0) == UNDEFINED);
  REQUIRE(buffer.dump() == "u:1:? u:2:? u:3:? u:4:? u:u:? ");
  REQUIRE(buffer.traverse(0) == "");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));

  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 3);
  REQUIRE(buffer.dump() == "u:1:A 0:2:B 1:3:C 2:u:D u:u:? ");
  REQUIRE(buffer.traverse(0) == "ABCD");

  // delete an element
  buffer.setPointer(0, 0);
  buffer.remove(0);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getTail(0) == 3);
  REQUIRE(buffer.dump() == "u:4:? u:2:B 1:3:C 2:u:D u:u:? ");
  REQUIRE(buffer.traverse(0) == "BCD");


  // insert an element
  buffer.insert(0, TestNode('E'));
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getTail(0) == 0);
  REQUIRE(buffer.dump() == "3:u:E u:2:B 1:3:C 2:0:D u:u:? ");
  REQUIRE(buffer.traverse(0) == "BCDE");

  // insert last element
  buffer.insert(0, TestNode('F'));
  REQUIRE(buffer.getAvailable() == UNDEFINED);
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getTail(0) == 4);
  REQUIRE(buffer.dump() == "3:4:E u:2:B 1:3:C 2:0:D 0:u:F ");
  REQUIRE(buffer.traverse(0) == "BCDEF");
}


TEST_CASE("Seek", "[buffer]")
{
  Buffer<TestNode, 4, 1> buffer;
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('D'));
  buffer.insert(0,TestNode('E'));
  buffer.insert(0,TestNode('F'));

  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getTail(0) == 3);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.dump() == "u:1:B 0:2:D 1:3:E 2:u:F ");
  REQUIRE(buffer.traverse(0) == "BDEF");
  
  buffer.seek(0,TestNode('A'));
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.get(0).value == 'B');
  buffer.seek(0,TestNode('B'));
  REQUIRE(buffer.get(0).value == 'B');
  buffer.seek(0,TestNode('C'));
  REQUIRE(buffer.get(0).value == 'D');
  buffer.seek(0,TestNode('D'));
  REQUIRE(buffer.get(0).value == 'D');
  buffer.seek(0,TestNode('E'));
  REQUIRE(buffer.get(0).value == 'E');
  buffer.seek(0,TestNode('F'));
  REQUIRE(buffer.get(0).value == 'F');
  buffer.seek(0,TestNode('G'));
  REQUIRE(buffer.get(0).value == 'F');
}

TEST_CASE("Event", "[event]")
{
  REQUIRE(sizeof(Event) == 8);

  Event event{1, Event::NoteOn, 2, 3, 4};
  REQUIRE(event.position == 1);
  REQUIRE(event.getType() == Event::NoteOn);
  REQUIRE(event.param1 == 3);
  REQUIRE(event.param2 == 4);

  REQUIRE(event > Event{0,Event::NoteOn,2,3,4});
  REQUIRE(event >= Event{1,Event::NoteOff,5,6,7});
  REQUIRE(event <= Event{2,Event::NoteOff,5,6,7});

	REQUIRE(event.isNew() == false);
	event.setNew(true);
	REQUIRE(event.isNew() == true);
	REQUIRE(event.getType() == Event::NoteOn);
	event.setNew(false);
	REQUIRE(event.isNew() == false);
}

TEST_CASE("Sequence", "[sequence]")
{
  Sequence sequence;
  sequence.addEvent(1, Event{10, Event::NoteOn, 0, 60, 1});
  sequence.addEvent(1, Event{20, Event::NoteOff, 0, 62, 2});
  sequence.addEvent(1, Event{30, Event::NoteOn, 0, 63, 3});
  sequence.returnToZero(1);
  sequence.addEvent(1, Event{15, Event::NoteOn, 0, 61, 4});
  sequence.addEvent(1, Event{35, Event::NoteOff, 0, 64, 5});
  REQUIRE(sequence.getBuffer().traverse(1) == "10:NoteOn,C4,1\n"
                                            "15:NoteOn,C#4,4\n"
					    "20:NoteOff,D4\n"
                                            "30:NoteOn,D#4,3\n"
					    "35:NoteOff,E4\n");
}

TEST_CASE("Sequence Seek", "[sequence]")
{
  Sequence sequence;
  SeekResult result;
  result = sequence.seek(4);
  REQUIRE(result.position == 24*4*4);
  REQUIRE(result.numerator == 4);
  REQUIRE(result.denominator == 4);
  sequence.addEvent(0, Event{0, Event::Meter, 3, 4, 0});
  sequence.addEvent(0, Event{24*3, Event::Meter, 4, 4, 0});
  sequence.addEvent(0, Event{24*7, Event::Meter, 6, 8, 0});
  result = sequence.seek(0);
  REQUIRE(result.position == 0);
  REQUIRE(result.numerator == 3);
  REQUIRE(result.denominator == 4);
  result = sequence.seek(1);
  REQUIRE(result.position == 24*3);
  REQUIRE(result.numerator == 4);
  REQUIRE(result.denominator == 4);
  result = sequence.seek(2);
  REQUIRE(result.position == 24*7);
  REQUIRE(result.numerator == 6);
  REQUIRE(result.denominator == 8);
  result = sequence.seek(3);
  REQUIRE(result.position == 24*10);
  REQUIRE(result.numerator == 6);
  REQUIRE(result.denominator == 8);
}

TEST_CASE("MIDIFile", "[midifile]")
{
  Sequence sequence;
  char trk0[] = "0:Tempo,599817\n"
		"0:Meter,4/4\n"
		"1920:Tempo,749835\n"
		"1920:Meter,3/4\n";
   char trk1[] = "0:NoteOn,C4,20\n"
		"100:NoteOff,C4\n"
		"240:NoteOn,D4,30\n"
		"340:NoteOff,D4\n"
		"480:NoteOn,E4,40\n"
		"580:NoteOff,E4\n"
		"720:NoteOn,F#4,50\n"
		"820:NoteOff,F#4\n"
	        "1920:NoteOn,C4,100\n"
		"2160:NoteOff,C4\n"
		"2400:NoteOn,C#4,100\n"
		"2640:NoteOff,C#4\n"
		"2880:NoteOn,D4,100\n"
		"3120:NoteOff,D4\n";
  char trk2[] = "120:NoteOn,C#3,60\n"
		"220:NoteOff,C#3\n"
		"360:NoteOn,D#3,70\n"
		"460:NoteOff,D#3\n"
		"600:NoteOn,F3,80\n"
		"700:NoteOff,F3\n"
		"840:NoteOn,G3,90\n"
		"940:NoteOff,G3\n";

  CFile file0 {"midi_0.mid"};
  REQUIRE(file0.isValid());
  MIDIFile midi_file0{file0};
  REQUIRE(midi_file0.import(sequence) == 0);
  REQUIRE(sequence.getTicks() == 480);
  REQUIRE(sequence.getBuffer().traverse(0) == trk0);
  REQUIRE(sequence.getBuffer().traverse(1) == trk1);
  REQUIRE(sequence.getBuffer().traverse(2) == trk2);

  CFile file1 {"midi_1.mid"};
  REQUIRE(file1.isValid());
  MIDIFile midi_file1{file1};
  REQUIRE(midi_file1.import(sequence) == 0);
  REQUIRE(sequence.getTicks() == 480);
  REQUIRE(sequence.getBuffer().traverse(0) == trk0);
  REQUIRE(sequence.getBuffer().traverse(1) == trk1);
  REQUIRE(sequence.getBuffer().traverse(2) == trk2);
}

TEST_CASE("Player Count", "[player]")
{
  Sequence sequence;
  TestMIDIPort midi_port;
  Recorder recorder{sequence, midi_port, midi_port};
  Player player{sequence, midi_port, recorder};
  sequence.addEvent(0, Event{0, Event::Meter, 3, 4, 0});
  sequence.addEvent(0, Event{24*3, Event::Meter, 6, 8, 0});
  player.play();

  REQUIRE(player.getMeasure() == 0);
  REQUIRE(player.getBeat() == 0);
  REQUIRE(player.getMeterN() == 4);
  REQUIRE(player.getMeterD() == 4);

  player.tick();

  REQUIRE(player.getMeterN() == 3);
  REQUIRE(player.getMeterD() == 4);

  for ( int i{0}; i < 23; i ++ ) player.tick();
  REQUIRE(player.getBeat() == 1);
  for ( int i{0}; i < 24; i ++ ) player.tick();
  REQUIRE(player.getBeat() == 2);
  for ( int i{0}; i < 24; i ++ ) player.tick();
  REQUIRE(player.getBeat() == 0);
  REQUIRE(player.getMeasure() == 1);
  for ( int i{0}; i < 24; i ++ ) player.tick();
  REQUIRE(player.getBeat() == 2);
  REQUIRE(player.getMeasure() == 1);
}

TEST_CASE("Player Play", "[player]")
{
  Sequence sequence;
  TestMIDIPort midi_port;
  TestTiming timing;

  CFile file {"midi_1.mid"};
  MIDIFile midi_file{file};
  midi_file.import(sequence);
  Recorder recorder{sequence, midi_port, midi_port};
  Player player{sequence, midi_port, recorder};
  player.play();
  for ( int i{0}; i < 480*7; i ++ )
  {
    midi_port.setTime(timing.getMicroseconds());
    player.tick();
    timing.delay(player.getDelay());
  }
  char result[] = "0:0:0:NoteOn,C4,20\n"
		  "125000:0:100:NoteOff,C4\n"
		  "150000:1:120:NoteOn,C#3,60\n"
		  "275000:1:220:NoteOff,C#3\n"
		  "300000:0:240:NoteOn,D4,30\n"
		  "425000:0:340:NoteOff,D4\n"
		  "450000:1:360:NoteOn,D#3,70\n"
		  "575000:1:460:NoteOff,D#3\n"
		  "600000:0:480:NoteOn,E4,40\n"
		  "725000:0:580:NoteOff,E4\n"
		  "750000:1:600:NoteOn,F3,80\n"
		  "875000:1:700:NoteOff,F3\n"
		  "900000:0:720:NoteOn,F#4,50\n"
		  "1025000:0:820:NoteOff,F#4\n"
		  "1050000:1:840:NoteOn,G3,90\n"
		  "1175000:1:940:NoteOff,G3\n"
		  "2400000:0:1920:NoteOn,C4,100\n"
		  "2774880:0:2160:NoteOff,C4\n"
		  "3149760:0:2400:NoteOn,C#4,100\n"
		  "3524640:0:2640:NoteOff,C#4\n"
		  "3899520:0:2880:NoteOn,D4,100\n"
		  "4274400:0:3120:NoteOff,D4\n";
  REQUIRE(midi_port.getLog() == result);

  midi_port.clear();
  player.seek(1);
  REQUIRE(player.getMeasure() == 1);
  REQUIRE(player.getBeat() == 0);
  REQUIRE(player.getMeterN() == 3);
  REQUIRE(player.getMeterD() == 4);
  REQUIRE(player.getBpm() == 800);
  for ( int i{0}; i < 480*3; i ++ )
  {
    midi_port.setTime(timing.getMicroseconds());
    player.tick();
    timing.delay(player.getDelay());
  }

  char seekrs[] = "4649280:0:1920:NoteOn,C4,100\n"
		  "5024160:0:2160:NoteOff,C4\n"
		  "5399040:0:2400:NoteOn,C#4,100\n"
		  "5773920:0:2640:NoteOff,C#4\n"
		  "6148800:0:2880:NoteOn,D4,100\n"
		  "6523680:0:3120:NoteOff,D4\n";
  REQUIRE(midi_port.getLog() == seekrs); 
}

TEST_CASE("Player loop", "[player]")
{
  TestMIDIPort midi_port;
  TestTiming timing;
  Sequence sequence;
  Recorder recorder{sequence, midi_port, midi_port};
  Player player{sequence, midi_port, recorder};
  player.setTempo(600000);
  Track &track {sequence.getTrack(1)};
  track.length = 4;
  sequence.addEvent(1, Event{24*0, Event::NoteOn, 0, 60, 30});
  sequence.addEvent(1, Event{24*1, Event::NoteOn, 0, 60, 40});
  sequence.addEvent(1, Event{24*2, Event::NoteOn, 0, 60, 50});
  sequence.addEvent(1, Event{24*3, Event::NoteOn, 0, 60, 60});
  player.play();
  
  char result[] = "0:0:0:NoteOn,C4,30\n"
		  "600000:0:24:NoteOn,C4,40\n"
		  "1200000:0:48:NoteOn,C4,50\n"
		  "1800000:0:72:NoteOn,C4,60\n"
		  "2400000:0:0:NoteOn,C4,30\n"
		  "3000000:0:24:NoteOn,C4,40\n"
		  "3600000:0:48:NoteOn,C4,50\n"
		  "4200000:0:72:NoteOn,C4,60\n"
		  "4800000:0:0:NoteOn,C4,30\n";
  for ( int i{0}; i < 24*9; i ++ )
  {
    midi_port.setTime(timing.getMicroseconds());
    player.tick();
    timing.delay(player.getDelay());
  }

  REQUIRE(midi_port.getLog() == result);
}

void playFor(const int ticks, TestMIDIPort &midi_port, TestTiming &timing, Player &player)
{
	for ( int i = 0; i < ticks; i ++ )
	{
	  midi_port.setTime(timing.getMicroseconds());
    player.tick();
    timing.delay(player.getDelay());
	}
}


TEST_CASE("Recorder passthru", "[recorder]")
{
	TestMIDIPort midi_port;
	TestTiming timing;
	Sequence sequence;
	Track &track {sequence.getTrack(1)};
	track.length = 4;
	track.state = Track::OVERDUBBING;
	Recorder recorder{sequence, midi_port, midi_port};
	recorder.setRecordTrack(1);
	recorder.setIsRecording(true);
  Player player{sequence, midi_port, recorder};
  player.setTempo(600000);
	player.play();

	playFor(22, midi_port, timing, player);
  recorder.receiveEvent(Event{0, Event::NoteOn, 0, 60, 60});
	playFor(4, midi_port, timing, player);
	REQUIRE(sequence.getBuffer().traverse(1) == "24:NoteOn,C4,60\n");
  recorder.receiveEvent(Event{0, Event::NoteOn, 0, 72, 72});
	playFor(24, midi_port, timing, player);
	REQUIRE(sequence.getBuffer().traverse(1) == "24:NoteOn,C5,72\n"
			                                        "24:NoteOn,C4,60\n");

	char result[] = "525000:0:0:NoteOn,C4,60\n"
									"625000:0:0:NoteOn,C5,72\n";
	REQUIRE(midi_port.getLog() == result);
}

/*TEST_CASE("Recorder delete", "[recorder]")
{
  TestMIDIPort midi_port;
  TestTiming timing;
  Sequence sequence;
  Recorder recorder{sequence, midi_port};
  Player player{sequence, midi_port, recorder};
  player.setTempo(600000);
  Track &track {sequence.getTrack(1)};
  track.length = 4;
  sequence.addEvent(1, Event{24*0, Event::NoteOn, 0, 60, 30});
  sequence.addEvent(1, Event{24*1, Event::NoteOn, 0, 60, 40});
  sequence.addEvent(1, Event{24*2, Event::NoteOn, 0, 60, 50});
  sequence.addEvent(1, Event{24*3, Event::NoteOn, 0, 60, 60});
  player.play();
  recorder.setRecordTrack(1);
  
  char result[] = "0:0:0:NoteOn,C4,30\n"
		  "600000:0:24:NoteOn,C4,40\n"
		  "1200000:0:48:NoteOn,C4,50\n"
		  "1800000:0:72:NoteOn,C4,60\n";
  for ( int i{0}; i < 24*9; i ++ )
  {
    midi_port.setTime(timing.getMicroseconds());
    if ( i >= 24*4 )
      recorder.setReplace(true);
    player.tick();
    timing.delay(player.getDelay());
  }

  REQUIRE(midi_port.getLog() == result);
}*/
