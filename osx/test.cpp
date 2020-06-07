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

const char *getNote(const int note)
{
  // 60 = C4
  const char *notes[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  return notes[note % 12];
}

int getOctave(const int note)
{
  return note / 12 - 1;
}

ostream& operator<<(ostream &o, const Event& event)
{
  o << event.position << ":";
  switch ( event.type )
  {
    case Event::NoteOn:
      o << static_cast<int>(event.channel) << ":NoteOn,"
	<< getNote(event.param1) << getOctave(event.param1) << ","
	<< static_cast<int>(event.param2);
      break;
    case Event::NoteOff:
      o << static_cast<int>(event.channel) << ":NoteOff,"
	<< getNote(event.param1) << getOctave(event.param1);
      break;
    case Event::Tempo:
      o << "Tempo," << fixed << setprecision(1) << event.getBpm();
      break;
    case Event::Meter:
      o << "Meter," << static_cast<int>(event.param0) << "/"
	<< static_cast<int>(event.param1);
      break;
    default:
      o << static_cast<int>(event.type)
        << "," << static_cast<int>(event.param1)
        << "," << static_cast<int>(event.param2);
  }
  o << endl;
  return o;
}

class TestTiming : public Timing
{
private:
  uint32_t microseconds;
public:
  TestTiming() : microseconds{0}
  {
  }

  uint32_t getMicroseconds()
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

  void send(const Event &event)
  {
    log << time << ":" << event;
  }

  void setTime(uint32_t t)
  {
    time = t;
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
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "C");

  buffer.insert(0, TestNode('C'));
  REQUIRE(buffer.getAvailable() == 2);
  REQUIRE(buffer.dump() == "1:u:C u:0:C u:3:? u:4:? u:u:? ");
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.traverse(0) == "CC");

  buffer.insert(0, TestNode('D'));
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.dump() == "1:2:C u:0:C 0:u:D u:4:? u:u:? ");
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.getPointer(0) == 2);
  REQUIRE(buffer.traverse(0) == "CCD");

  buffer.insert(0, TestNode('A'));
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.dump() == "1:2:C 3:0:C 0:u:D u:1:A u:u:? ");
  REQUIRE(buffer.getHead(0) == 3);
  REQUIRE(buffer.getPointer(0) == 2);
  REQUIRE(buffer.traverse(0) == "ACCD");

  buffer.insert(0, TestNode('B'));
  REQUIRE(buffer.getAvailable() == UNDEFINED);
  REQUIRE(buffer.dump() == "1:2:C 4:0:C 0:u:D u:4:A 3:1:B ");
  REQUIRE(buffer.getHead(0) == 3);
  REQUIRE(buffer.getPointer(0) == 2);
  REQUIRE(buffer.traverse(0) == "ABCCD");
}

TEST_CASE("Buffer full", "[buffer]")
{
  Buffer<TestNode,4, 1> buffer;
  REQUIRE(buffer.insert(0, TestNode('A')) == true);
  REQUIRE(buffer.insert(0, TestNode('B')) == true);
  REQUIRE(buffer.insert(0, TestNode('C')) == true);
  REQUIRE(buffer.insert(0, TestNode('D')) == true);
  REQUIRE(buffer.insert(0, TestNode(5)) == false);
}

TEST_CASE("Buffer clear", "[buffer]")
{
  Buffer<TestNode,4, 1> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));
  buffer.clear();
  REQUIRE(buffer.getHead(0) == UNDEFINED);
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
  REQUIRE(buffer.getHead(1) == 1);
  REQUIRE(buffer.getPointer(0) == 0);
  REQUIRE(buffer.getPointer(1) == 1);
  REQUIRE(buffer.traverse(0) == "A");
  REQUIRE(buffer.traverse(1) == "C");

  buffer.insert(1, TestNode('B'));
  REQUIRE(buffer.dump() == "u:u:A 2:u:C u:1:B ");
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.getHead(1) == 2);
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
  REQUIRE(buffer.getPointer(0) == 1);
  REQUIRE(buffer.dump() == "u:u:? u:2:B 1:3:C 2:u:D ");
  REQUIRE(buffer.traverse(0) == "BCD");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));

  // delete from middle
  buffer.setPointer(0, 1);
  buffer.remove(0);
  REQUIRE(buffer.getHead(0) == 0);
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
  REQUIRE(buffer.getPointer(0) == 2);
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
  REQUIRE(buffer.traverse(0) == "A");
  
  // delete only element
  buffer.remove(0);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead(0) == UNDEFINED);
  REQUIRE(buffer.dump() == "u:1:? u:2:? u:3:? u:4:? u:u:? ");
  REQUIRE(buffer.traverse(0) == "");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));

  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead(0) == 0);
  REQUIRE(buffer.dump() == "u:1:A 0:2:B 1:3:C 2:u:D u:u:? ");
  REQUIRE(buffer.traverse(0) == "ABCD");

  // delete an element
  buffer.setPointer(0, 0);
  buffer.remove(0);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.dump() == "u:4:? u:2:B 1:3:C 2:u:D u:u:? ");
  REQUIRE(buffer.traverse(0) == "BCD");


  // insert an element
  buffer.insert(0, TestNode('E'));
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead(0) == 1);
  REQUIRE(buffer.dump() == "3:u:E u:2:B 1:3:C 2:0:D u:u:? ");
  REQUIRE(buffer.traverse(0) == "BCDE");

  // insert last element
  buffer.insert(0, TestNode('F'));
  REQUIRE(buffer.getAvailable() == UNDEFINED);
  REQUIRE(buffer.getHead(0) == 1);
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
  REQUIRE(buffer.getPointer(0) == 3);
  REQUIRE(buffer.traverse(0) == "BDEF");
  
  buffer.seek(0,TestNode('A'));
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
  REQUIRE(event.type == Event::NoteOn);
  REQUIRE(event.channel == 2);
  REQUIRE(event.param1 == 3);
  REQUIRE(event.param2 == 4);

  REQUIRE(event > Event{0,Event::NoteOn,2,3,4});
  REQUIRE(event >= Event{1,Event::NoteOff,5,6,7});
  REQUIRE(event <= Event{2,Event::NoteOff,5,6,7});
}

TEST_CASE("Sequence", "[sequence]")
{
  Sequence sequence;
  sequence.addEvent(1, Event{10, Event::NoteOn, 1, 60, 1});
  sequence.addEvent(1, Event{20, Event::NoteOff, 2, 62, 2});
  sequence.addEvent(1, Event{30, Event::NoteOn, 3, 63, 3});
  sequence.returnToZero();
  sequence.addEvent(1, Event{15, Event::NoteOn, 4, 61, 4});
  sequence.addEvent(1, Event{35, Event::NoteOff, 5, 64, 5});
  REQUIRE(sequence.getBuffer().traverse(1) == "10:1:NoteOn,C4,1\n"
                                            "15:4:NoteOn,C#4,4\n"
					    "20:2:NoteOff,D4\n"
                                            "30:3:NoteOn,D#4,3\n"
					    "35:5:NoteOff,E4\n");
}

TEST_CASE("MIDIFile", "[midifile]")
{
  Sequence sequence;
  char trk0[] = "0:Tempo,100.0\n"
		"0:Meter,4/4\n"
		"1920:Tempo,80.0\n"
		"1920:Meter,3/4\n";
  char trk1[] = "0:0:NoteOn,C4,20\n"
		"100:0:NoteOff,C4\n"
		"120:1:NoteOn,C#3,60\n"
		"220:1:NoteOff,C#3\n"
		"240:0:NoteOn,D4,30\n"
		"340:0:NoteOff,D4\n"
		"360:1:NoteOn,D#3,70\n"
		"460:1:NoteOff,D#3\n"
		"480:0:NoteOn,E4,40\n"
		"580:0:NoteOff,E4\n"
		"600:1:NoteOn,F3,80\n"
		"700:1:NoteOff,F3\n"
		"720:0:NoteOn,F#4,50\n"
		"820:0:NoteOff,F#4\n"
		"840:1:NoteOn,G3,90\n"
		"940:1:NoteOff,G3\n";

  CFile file0 {"midi_0.mid"};
  REQUIRE(file0.isValid());
  MIDIFile midi_file0{file0};
  REQUIRE(midi_file0.import(sequence) == 0);
  REQUIRE(sequence.getTicks() == 480);
  REQUIRE(sequence.getBuffer().traverse(0) == trk0);
  REQUIRE(sequence.getBuffer().traverse(1) == trk1);

  CFile file1 {"midi_1.mid"};
  REQUIRE(file1.isValid());
  MIDIFile midi_file1{file1};
  REQUIRE(midi_file1.import(sequence) == 0);
  REQUIRE(sequence.getTicks() == 480);
  REQUIRE(sequence.getBuffer().traverse(0) == trk0);
  REQUIRE(sequence.getBuffer().traverse(1) == trk1);
}

TEST_CASE("Player Count", "[player]")
{
  Sequence sequence;
  TestTiming timing;
  TestMIDIPort midi_port;
  Player player{sequence, timing, midi_port};
  sequence.addEvent(0, Event{0, Event::Meter, 3, 4, 0});
  sequence.returnToZero();

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
}

TEST_CASE("Player Play", "[player]")
{
  Sequence sequence;
  TestTiming timing;
  TestMIDIPort midi_port;

  CFile file {"midi_1.mid"};
  MIDIFile midi_file{file};
  midi_file.import(sequence);
  Player player{sequence, timing, midi_port};
  for ( int i{0}; i < 480*4; i ++ )
  {
    midi_port.setTime(timing.getMicroseconds());
    player.tick();
  }
  char result[] = "0:0:0:NoteOn,C4,20\n"
		  "125000:100:0:NoteOff,C4\n"
		  "150000:120:1:NoteOn,C#3,60\n"
		  "275000:220:1:NoteOff,C#3\n"
		  "300000:240:0:NoteOn,D4,30\n"
		  "425000:340:0:NoteOff,D4\n"
		  "450000:360:1:NoteOn,D#3,70\n"
		  "575000:460:1:NoteOff,D#3\n"
		  "600000:480:0:NoteOn,E4,40\n"
		  "725000:580:0:NoteOff,E4\n"
		  "750000:600:1:NoteOn,F3,80\n"
		  "875000:700:1:NoteOff,F3\n"
		  "900000:720:0:NoteOn,F#4,50\n"
		  "1025000:820:0:NoteOff,F#4\n"
		  "1050000:840:1:NoteOn,G3,90\n"
		  "1175000:940:1:NoteOff,G3\n";
  REQUIRE(midi_port.getLog() == result);
}
