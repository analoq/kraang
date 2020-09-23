#ifndef BUFFER_HPP
#define BUFFER_HPP
#ifdef CATCH_CONFIG_MAIN
#include <sstream>
#include <algorithm>
using namespace std;
#endif
#include <stdint.h>
#include <assert.h>

static const int16_t UNDEFINED {-1};

template<class T>
class Node
{
public:
  int16_t next;
  int16_t prev;
  T data;

  Node() : next{UNDEFINED}, prev{UNDEFINED}
  {
  }

  Node(T data) : data{data}, next{UNDEFINED}, prev{UNDEFINED}
  {
  }
};

template<class T, uint16_t SIZE, uint8_t TRACKS>
class Buffer
{
private:
  int16_t available;
  int16_t pointer[TRACKS];
  int16_t head[TRACKS];
  int16_t tail[TRACKS];
  Node<T> buffer[SIZE];

  struct SearchResult
  {
    int16_t last;
    int16_t curr;
    bool go_right;
  };

  const SearchResult search(const uint8_t track, const T &data)
  {
    SearchResult result;
    result.last = UNDEFINED;
    if ( pointer[track] == UNDEFINED )
      result.curr = tail[track];
    else
      result.curr = pointer[track];
    result.go_right = data >= buffer[result.curr].data;
    while ( true )
    {
      const T &current{buffer[result.curr].data};
      if ( result.go_right )
      {
	if (data <= current )
	  break;
      
	if ( result.curr == UNDEFINED )
	  break;
	result.last = result.curr;
	result.curr = buffer[result.curr].next;
      }
      else
      {
	if ( data >= current )
	  break;

	if ( result.curr == UNDEFINED )
	  break;
	result.last = result.curr;
	result.curr = buffer[result.curr].prev;
      }
    }
    return result;
 }

public:
  Buffer()
  {
    clear();
  }

  void clear()
  {
    for ( uint32_t i{0}; i < SIZE; ++i )
    {
      buffer[i] = Node<T>{};
      if ( i == SIZE - 1 )
	buffer[i].next = UNDEFINED;
      else
        buffer[i].next = i + 1;
    }

    for ( uint8_t i{0}; i < TRACKS; ++i )
    {
      pointer[i] = UNDEFINED;
      head[i] = UNDEFINED;
      tail[i] = UNDEFINED;
    }
    available = 0;
  }

  void returnToZero(const uint8_t track)
  {
    pointer[track] = head[track];
  }

  bool notUndefined(const uint8_t track) const
  {
    return pointer[track] != UNDEFINED;
  }

  void setUndefined(const uint8_t track)
  {
    pointer[track] = UNDEFINED;
  }

  void next(const uint8_t track)
  {
    Node<T> &current {buffer[pointer[track]]};
    pointer[track] = current.next;
  }

  const T &get(const uint8_t track) const
  {
    assert(pointer[track] != UNDEFINED);
    return buffer[pointer[track]].data;
  }

  void seek(const uint8_t track, const T &data)
  {
    SearchResult result {search(track, data)};
    if ( result.curr == UNDEFINED )
      pointer[track] = result.last;
    else
      pointer[track] = result.curr;
  }

  bool insert(const uint8_t track, const T &data)
  {
    assert(track < TRACKS);
    if ( available == UNDEFINED )
      return false;
    buffer[available].data = data;
    const int16_t new_node {available};
    available = buffer[available].next;
    if ( head[track] == UNDEFINED )
    {
      head[track] = new_node;
      tail[track] = new_node;
      pointer[track] = new_node;
      buffer[new_node].prev = UNDEFINED;
      buffer[new_node].next = UNDEFINED;
    }
    else
    {
      SearchResult result {search(track, data)};
      int16_t curr = result.curr;
      int16_t last = result.last;
      bool go_right = result.go_right;

      if ( go_right && curr == UNDEFINED )
      {
	// add to end
	if ( last != UNDEFINED )
	  buffer[last].next = new_node;
	buffer[new_node].prev = last;
	buffer[new_node].next = UNDEFINED;
	pointer[track] = new_node;
	tail[track] = new_node;
      }
      else if ( !go_right && curr == UNDEFINED )
      {
	// add to beginning
	buffer[new_node].prev = UNDEFINED;
	buffer[new_node].next = head[track];
	buffer[head[track]].prev = new_node;
	head[track] = new_node;
      }
      else if ( go_right )
      {
	if ( buffer[curr].prev != UNDEFINED )
	  buffer[buffer[curr].prev].next = new_node;
	buffer[new_node].prev = buffer[curr].prev;
	buffer[new_node].next = curr;
	buffer[curr].prev = new_node;
	pointer[track] = curr;
	if ( head[track] == pointer[track] )
	  head[track] = new_node;
      }
      else if ( !go_right )
      {
	buffer[new_node].prev = curr;
	buffer[new_node].next = last;
	if ( buffer[curr].next != UNDEFINED )
	  buffer[buffer[curr].next].prev = new_node;
	buffer[curr].next = new_node;
      }
    }
    return true;
  }
  
  void remove(const uint8_t track)
  {
    assert(track < TRACKS);
    if ( head[track] == UNDEFINED )
      return;
    const int16_t curr {pointer[track]};
    if ( curr == tail[track] )
      tail[track] = buffer[curr].prev;
    if ( pointer[track] == head[track] )
    {
      head[track] = buffer[head[track]].next;
      pointer[track] = head[track];
      if ( buffer[curr].next != UNDEFINED )
	buffer[buffer[curr].next].prev = UNDEFINED;
    }
    else
    {
      if ( buffer[curr].prev != UNDEFINED )
	buffer[buffer[curr].prev].next = buffer[curr].next;
      if ( buffer[curr].next != UNDEFINED )
      {
        buffer[buffer[curr].next].prev = buffer[curr].prev;
	pointer[track] = buffer[curr].next;
      }
      else
	pointer[track] = UNDEFINED; //buffer[curr].prev;
    }

    buffer[curr] = Node<T>{};
    buffer[curr].next = available;
    available = curr;
  }

  #ifdef CATCH_CONFIG_MAIN
  int16_t getHead(uint8_t track) const
  {
    return head[track];
  }

  int16_t getTail(uint8_t track) const
  {
    return tail[track];
  }

  int16_t getPointer(uint8_t track) const
  {
    return pointer[track];
  }

  int16_t getAvailable() const
  {
    return available;
  }

  void setPointer(uint8_t track, int16_t index)
  {
    pointer[track] = index;
  }

  string traverse(uint8_t track) const
  {
    stringstream result;
    for ( int16_t index {head[track]};
	  index != UNDEFINED;
	  index = buffer[index].next)
    {
      result << buffer[index].data;
    }
    return result.str();
  }

  string dump() const
  {
    stringstream result;
    for (int i {0}; i < min(static_cast<uint16_t>(32), SIZE); i ++ )
    {
      if (buffer[i].prev == UNDEFINED)
	result << 'u';
      else
	result << buffer[i].prev;
      result << ':';
      if (buffer[i].next == UNDEFINED)
	result << 'u';
      else
	result << buffer[i].next;
      result << ":" << buffer[i].data << " ";
    }
    return result.str();
  }
  #endif
};
#endif
