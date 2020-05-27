/*
  process launch -- -b
  breakpoint set --file EventBuffer.hpp --line 58
*/
#define CATCH_CONFIG_MAIN
#include <iostream>
#include "catch.hpp"
#include "../Buffer.hpp"
#include "../Sequence.hpp"
#include "../MIDIFile.hpp"

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

  bool operator <(const TestNode &a) const
  {
    return value < a.value;
  }

  bool operator ==(const TestNode &a) const
  {
    return value == a.value;
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
  o << event.position << ":" << static_cast<int>(event.channel) << ":";
  switch ( event.type )
  {
    case Event::NoteOn:
      o << "NoteOn," << getNote(event.param1) << getOctave(event.param1) << "," << static_cast<int>(event.param2);
      break;
    case Event::NoteOff:
      o << "NoteOff," << getNote(event.param1) << getOctave(event.param1) << "," << static_cast<int>(event.param2);
      break;
    default:
      o << static_cast<int>(event.type)
        << "," << static_cast<int>(event.param1)
        << "," << static_cast<int>(event.param2);
  }
  o << endl;
  return o;
}

template<class T, uint32_t N>
string traverse(Buffer<T,N> &buffer)
{
  stringstream result;
  for ( const T &node : buffer )
    result << node;
  return result.str();
}


TEST_CASE("New buffer is empty", "[buffer]")
{
  Buffer<TestNode, 2> buffer;
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == -1);
  REQUIRE(buffer.getTail() == -1);
  REQUIRE(buffer.dump() == "-1:1:? -1:-1:? ");
  REQUIRE(traverse(buffer) == "");
}

TEST_CASE("Buffer out of range", "[buffer]")
{
  Buffer<TestNode, 4> buffer;
  REQUIRE(buffer.insert(4,TestNode('A')) == -1);
  REQUIRE(buffer.remove(4) == false);
}

TEST_CASE("Buffer full", "[buffer]")
{
  Buffer<TestNode,4> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));
  REQUIRE(buffer.insert(0,TestNode(5)) == -1);
}

TEST_CASE("Buffer clear", "[buffer]")
{
  Buffer<TestNode,4> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(0,TestNode('D'));
  buffer.clear();
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == -1);
  REQUIRE(buffer.getTail() == -1);
  REQUIRE(traverse(buffer) == "");
  REQUIRE(buffer.dump() == "-1:1:? -1:2:? -1:3:? -1:-1:? ");
}


TEST_CASE("Buffer insertions", "[buffer]")
{
  Buffer<TestNode, 4> buffer;
  // insert to empty list
  REQUIRE(buffer.insert(0,TestNode('A')) == 0);
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.getAvailable() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.dump() == "-1:-1:A -1:2:? -1:3:? -1:-1:? ");
  REQUIRE(traverse(buffer) == "A");
  // insert at head
  REQUIRE(buffer.insert(0,TestNode('B')) == 1);
  REQUIRE(buffer.getAvailable() == 2);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.dump() == "1:-1:A -1:0:B -1:3:? -1:-1:? ");
  REQUIRE(traverse(buffer) == "BA");
  // insert at tail
  REQUIRE(buffer.insert(0,TestNode('C')) == 2);
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.dump() == "2:-1:A -1:2:B 1:0:C -1:-1:? ");
  REQUIRE(traverse(buffer) == "BCA");
  // insert at end
  REQUIRE(buffer.insert(3,TestNode('D')) == 3);
  REQUIRE(buffer.getAvailable() == -1);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(buffer.dump() == "2:3:A -1:2:B 1:0:C 0:-1:D ");
  REQUIRE(traverse(buffer) == "BCAD");
}


TEST_CASE("Buffer deletions", "[buffer]")
{
  Buffer<TestNode, 4> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));
  // "2:3:A -1:2:B 1:0:C 0:-1:D  BCAD
  
  // delete from head
  REQUIRE(buffer.remove(1) == true);
  REQUIRE(buffer.getAvailable() == 1);
  REQUIRE(buffer.getHead() == 2);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "CAD");
  REQUIRE(buffer.dump() == "2:3:A -1:-1:? -1:0:C 0:-1:D ");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));

  // delete from middle
  REQUIRE(buffer.remove(0) == true);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "BCD");
  REQUIRE(buffer.dump() == "-1:-1:? -1:2:B 1:3:C 2:-1:D ");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));

  // delete from end
  REQUIRE(buffer.remove(3) == true);
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(traverse(buffer) == "BCA");
  REQUIRE(buffer.dump() == "2:-1:A -1:2:B 1:0:C -1:-1:? ");

  // delete deleted
  REQUIRE(buffer.remove(3) == true);
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(traverse(buffer) == "BCA");
  //REQUIRE(buffer.dump() == "2:-1:A -1:2:B 1:0:C -1:3:? ");
}

TEST_CASE("Buffer deletion/insertion", "[buffer]")
{
  Buffer<TestNode, 5> buffer;
  // add only element
  buffer.insert(TestNode('A'));
  REQUIRE(buffer.getAvailable() == 1);
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(traverse(buffer) == "A");
  
  // delete only element
  REQUIRE(buffer.remove(0) == true);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == -1);
  REQUIRE(buffer.getTail() == -1);
  REQUIRE(traverse(buffer) == "");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));

  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "BCAD");
  REQUIRE(buffer.dump() == "2:3:A -1:2:B 1:0:C 0:-1:D -1:-1:? ");

  // delete an element
  REQUIRE(buffer.remove(0) == true);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "BCD");
  REQUIRE(buffer.dump() == "-1:4:? -1:2:B 1:3:C 2:-1:D -1:-1:? ");

  // insert an element
  REQUIRE(buffer.insert(TestNode('E')) == 0);
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(traverse(buffer) == "BCDE");
  REQUIRE(buffer.dump() == "3:-1:E -1:2:B 1:3:C 2:0:D -1:-1:? ");

  // insert last element
  REQUIRE(buffer.insert(TestNode('F')) == 4);
  REQUIRE(buffer.getAvailable() == -1);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 4);
  REQUIRE(traverse(buffer) == "BCDEF");
  REQUIRE(buffer.dump() == "3:4:E -1:2:B 1:3:C 2:0:D 0:-1:F ");
}

TEST_CASE("Compare", "[test]")
{
  REQUIRE((TestNode('A') < TestNode('B')) == true);
  REQUIRE((TestNode('B') < TestNode('A')) == false);
}

TEST_CASE("Buffer insert sorted", "[buffer]")
{
  Buffer<TestNode, 7> buffer;
  REQUIRE(buffer.insert_sorted(TestNode('E')) == 0);
  REQUIRE(buffer.insert_sorted(TestNode('C')) == 1);
  REQUIRE(buffer.insert_sorted(TestNode('A')) == 2);
  REQUIRE(traverse(buffer) == "ACE");

  REQUIRE(buffer.insert_sorted(TestNode('B')) == 3);
  REQUIRE(buffer.insert_sorted(3, TestNode('C')) == 4);
  REQUIRE(buffer.insert_sorted(4, TestNode('D')) == 5);
  REQUIRE(buffer.insert_sorted(5, TestNode('F')) == 6);
  REQUIRE(traverse(buffer) == "ABCCDEF");
}

TEST_CASE("Swap", "[buffer]")
{
  Buffer<TestNode, 5> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));
  // "2:3:A -1:2:B 1:0:C 0:-1:D -1:-1:? BCAD
 
  buffer.doSwap(0,1);
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.dump() == "-1:2:B 2:3:A 0:1:C 1:-1:D -1:-1:? ");
  REQUIRE(traverse(buffer) == "BCAD");

  buffer.doSwap(1,2);
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.dump() == "-1:1:B 0:2:C 1:3:A 2:-1:D -1:-1:? ");
  REQUIRE(traverse(buffer) == "BCAD");
}

TEST_CASE("Sort", "[buffer]")
{
  Buffer<TestNode, 4> buffer;
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));
  // "2:3:A -1:2:B 1:0:C 0:-1:D  BCAD

  buffer.sort();
  REQUIRE(buffer.getAvailable() == -1);
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "BCAD");
  REQUIRE(buffer.dump() == "-1:1:B 0:2:C 1:3:A 2:-1:D ");
}

TEST_CASE("Search", "[buffer]")
{
  Buffer<TestNode, 4> buffer;
  buffer.insert(TestNode('B'));
  buffer.insert(TestNode('D'));
  buffer.insert(TestNode('E'));
  buffer.insert(TestNode('F'));

  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "BDEF");
  // can't find if unsorted
  REQUIRE(buffer.search(TestNode('B')) == -1);
  // find A
  buffer.sort();
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(traverse(buffer) == "BDEF");
  REQUIRE(buffer.search(TestNode('A')) == 0);
  REQUIRE(buffer.search(TestNode('B')) == 0);
  REQUIRE(buffer.search(TestNode('C')) == 1);
  REQUIRE(buffer.search(TestNode('D')) == 1);
  REQUIRE(buffer.search(TestNode('E')) == 2);
  REQUIRE(buffer.search(TestNode('F')) == 3);
  REQUIRE(buffer.search(TestNode('G')) == 3);
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

  REQUIRE(event < Event{2,Event::NoteOn,2,3,4});
  REQUIRE(event == Event{1,Event::NoteOff,5,6,7});
}

TEST_CASE("Sequence", "[sequence]")
{
  Sequence sequence;
  sequence.addEvent(Event{10, Event::NoteOn, 1, 60, 1});
  sequence.addEvent(Event{20, Event::NoteOff, 2, 62, 2});
  sequence.addEvent(Event{30, Event::NoteOn, 3, 63, 3});
  sequence.returnToZero();
  sequence.addEvent(Event{15, Event::NoteOn, 4, 61, 4});
  sequence.addEvent(Event{35, Event::NoteOff, 5, 64, 5});
  REQUIRE(traverse(sequence.getBuffer()) == "10:1:NoteOn,C4,1\n"
                                            "15:4:NoteOn,C#4,4\n"
					    "20:2:NoteOff,D4,2\n"
                                            "30:3:NoteOn,D#4,3\n"
					    "35:5:NoteOff,E4,5\n");
}

TEST_CASE("MIDIFile", "[midifile]")
{
  Sequence sequence;
  FILE *fp = fopen("midi_0.mid", "rb");
  REQUIRE(fp != NULL);

  MIDIFile midi_file{fp};
  REQUIRE(midi_file.import(sequence) == 0);

  REQUIRE(sequence.getTicks() == 960);
  REQUIRE(sequence.getBpm() == 120.0);
  cout << traverse(sequence.getBuffer()) << endl;
}
