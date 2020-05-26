/*
  process launch -- -b
  breakpoint set --file EventBuffer.hpp --line 58

DONE:
- Inserting
- Deleting
- Swapping
- Sorting
- Github
- Searching
TODO:
- Arduino Test
*/
#define CATCH_CONFIG_MAIN
#include <iostream>
#include "catch.hpp"
#include "Buffer.hpp"

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


TEST_CASE("New buffer is empty", "[buffer]")
{
  Buffer<TestNode, 2> buffer;
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == -1);
  REQUIRE(buffer.getTail() == -1);
  REQUIRE(buffer.dump() == "-1:1:? -1:-1:? ");
  REQUIRE(buffer.traverse() == "");
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
  REQUIRE(buffer.traverse() == "");
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
  REQUIRE(buffer.traverse() == "A");
  // insert at head
  REQUIRE(buffer.insert(0,TestNode('B')) == 1);
  REQUIRE(buffer.getAvailable() == 2);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.dump() == "1:-1:A -1:0:B -1:3:? -1:-1:? ");
  REQUIRE(buffer.traverse() == "BA");
  // insert at tail
  REQUIRE(buffer.insert(0,TestNode('C')) == 2);
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.dump() == "2:-1:A -1:2:B 1:0:C -1:-1:? ");
  REQUIRE(buffer.traverse() == "BCA");
  // insert at end
  REQUIRE(buffer.insert(3,TestNode('D')) == 3);
  REQUIRE(buffer.getAvailable() == -1);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(buffer.dump() == "2:3:A -1:2:B 1:0:C 0:-1:D ");
  REQUIRE(buffer.traverse() == "BCAD");
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
  REQUIRE(buffer.traverse() == "CAD");
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
  REQUIRE(buffer.traverse() == "BCD");
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
  REQUIRE(buffer.traverse() == "BCA");
  REQUIRE(buffer.dump() == "2:-1:A -1:2:B 1:0:C -1:-1:? ");

  // delete deleted
  REQUIRE(buffer.remove(3) == true);
  REQUIRE(buffer.getAvailable() == 3);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.traverse() == "BCA");
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
  REQUIRE(buffer.traverse() == "A");
  
  // delete only element
  REQUIRE(buffer.remove(0) == true);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == -1);
  REQUIRE(buffer.getTail() == -1);
  REQUIRE(buffer.traverse() == "");

  buffer.clear();
  buffer.insert(0,TestNode('A'));
  buffer.insert(0,TestNode('B'));
  buffer.insert(0,TestNode('C'));
  buffer.insert(3,TestNode('D'));

  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(buffer.traverse() == "BCAD");
  REQUIRE(buffer.dump() == "2:3:A -1:2:B 1:0:C 0:-1:D -1:-1:? ");

  // delete an element
  REQUIRE(buffer.remove(0) == true);
  REQUIRE(buffer.getAvailable() == 0);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(buffer.traverse() == "BCD");
  REQUIRE(buffer.dump() == "-1:4:? -1:2:B 1:3:C 2:-1:D -1:-1:? ");

  // insert an element
  REQUIRE(buffer.insert(TestNode('E')) == 0);
  REQUIRE(buffer.getAvailable() == 4);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 0);
  REQUIRE(buffer.traverse() == "BCDE");
  REQUIRE(buffer.dump() == "3:-1:E -1:2:B 1:3:C 2:0:D -1:-1:? ");

  // insert last element
  REQUIRE(buffer.insert(TestNode('F')) == 4);
  REQUIRE(buffer.getAvailable() == -1);
  REQUIRE(buffer.getHead() == 1);
  REQUIRE(buffer.getTail() == 4);
  REQUIRE(buffer.traverse() == "BCDEF");
  REQUIRE(buffer.dump() == "3:4:E -1:2:B 1:3:C 2:0:D 0:-1:F ");
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
  REQUIRE(buffer.traverse() == "BCAD");

  buffer.doSwap(1,2);
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.dump() == "-1:1:B 0:2:C 1:3:A 2:-1:D -1:-1:? ");
  REQUIRE(buffer.traverse() == "BCAD");
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
  REQUIRE(buffer.traverse() == "BCAD");
  REQUIRE(buffer.dump() == "-1:1:B 0:2:C 1:3:A 2:-1:D ");
}

TEST_CASE("Compare", "[test]")
{
  REQUIRE((TestNode('A') < TestNode('B')) == true);
  REQUIRE((TestNode('B') < TestNode('A')) == false);
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
  REQUIRE(buffer.traverse() == "BDEF");
  // can't find if unsorted
  REQUIRE(buffer.search(TestNode('B')) == -1);
  // find A
  buffer.sort();
  REQUIRE(buffer.getHead() == 0);
  REQUIRE(buffer.getTail() == 3);
  REQUIRE(buffer.traverse() == "BDEF");
  REQUIRE(buffer.search(TestNode('A')) == 0);
  REQUIRE(buffer.search(TestNode('B')) == 0);
  REQUIRE(buffer.search(TestNode('C')) == 1);
  REQUIRE(buffer.search(TestNode('D')) == 1);
  REQUIRE(buffer.search(TestNode('E')) == 2);
  REQUIRE(buffer.search(TestNode('F')) == 3);
  REQUIRE(buffer.search(TestNode('G')) == 3);
}
