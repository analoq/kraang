#ifndef BUFFER_HPP
#define BUFFER_HPP
#ifdef CATCH_CONFIG_MAIN
#include <sstream>
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

template<class T, uint16_t SIZE>
class Buffer
{
private:
  int32_t available;
  int32_t head;
  int32_t tail;
  bool sorted;
  Node<T> buffer[SIZE];
  int32_t iterator;

  void swap(const int32_t i, const int32_t j)
  {
    // side-effects: does not update tail or available
    if ( i == j )
      return;

    // update indices
    const int32_t ip {buffer[i].prev};
    const int32_t ipn {buffer[ip].next};
    const int32_t in {buffer[i].next};
    const int32_t inp {buffer[in].prev};
    const int32_t jp {buffer[j].prev};
    const int32_t jpn {buffer[jp].next};
    const int32_t jn {buffer[j].next};
    const int32_t jnp {buffer[jn].prev};

    buffer[i].prev = jp;
    buffer[i].next = ip;
    buffer[j].prev = jn;
    buffer[j].next = in;

    if ( buffer[i].next != UNDEFINED )
      buffer[buffer[i].next].prev = i;
    if ( buffer[j].prev != UNDEFINED )
      buffer[buffer[j].prev].next = j;
    if ( buffer[j].next != UNDEFINED )
      buffer[buffer[j].next].prev = j;
    if ( buffer[i].prev != UNDEFINED )
    buffer[buffer[i].prev].next = i;

    // swap
    T temp {buffer[i].data};
    buffer[i].data = buffer[j].data;
    buffer[j].data = temp;

    // update head
    if ( j == head )
      head = i;
    else if ( i == head )
      head = j;
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
    iterator = UNDEFINED;
    tail = UNDEFINED;
    available = 0;
    head = UNDEFINED;
    sorted = false;
  }

  const int32_t insert(const uint32_t index, const T data)
  {
    if ( index >= SIZE )
      return -1;
    if ( available == UNDEFINED )
      return -1;

    const int32_t curr_available {available};
    const int32_t next_available {buffer[available].next};
    if ( head == UNDEFINED && tail == UNDEFINED )
    {
      // new node at beginning
      buffer[0] = Node<T>{data};
      // update endices
      tail = 0;
      head = 0;
    }
    else
    {
      if ( index == head )
      {
	buffer[available] = Node<T>{data};
	buffer[available].next = head;
	buffer[head].prev = available;
	head = available;
      }
      else if ( index == available )
      {
	buffer[available] = Node<T>{data};
	buffer[available].prev = tail;
	buffer[tail].next = available;
	tail = available;
      }
      else
      {
	buffer[available] = Node<T>{data};
	buffer[available].prev = buffer[index].prev;
	buffer[available].next = index;
	buffer[buffer[index].prev].next = available;
	buffer[index].prev = available;
      }
    }
    available = next_available;
    sorted = false;
    return curr_available;
  }

  const int32_t insert(const T data)
  {
    return insert(available, data);
  }

  const int32_t insert_sorted(int32_t index, const T data)
  {
    while ( index != UNDEFINED && !(data < buffer[index].data) )
      index = buffer[index].next;
    if ( index == UNDEFINED )
      return insert(data);
    else
      return insert(index, data); 
  }

  const int32_t insert_sorted(const T data)
  {
    if ( head == UNDEFINED )
      return insert(data);
    return insert_sorted(head, data);
  }

  const bool remove(const uint32_t index)
  {
    if ( index >= SIZE )
      return false;

    if ( index == head && index == tail )
    {
      head = UNDEFINED;
      tail = UNDEFINED;
    }
    else if ( index == head )
    {
      head = buffer[head].next;
      buffer[head].prev = UNDEFINED;
    }
    else if ( index == tail )
    {
      tail = buffer[tail].prev;
      buffer[tail].next = UNDEFINED;
    }
    else if ( buffer[index].prev != UNDEFINED && buffer[index].next != UNDEFINED )
    {
      buffer[buffer[index].prev].next = buffer[index].next;
      buffer[buffer[index].next].prev = buffer[index].prev;
    }
    buffer[index] = Node<T>{};
    buffer[index].next = available;
    available = index;
    sorted = false;
    return true;
  }

  const T& operator[](const int16_t index) const
  {
    return buffer[index].data;
  }

  const int32_t getNext(const int16_t index) const
  {
    return buffer[index].next;
  }

  int32_t getHead() const
  {
    return head;
  }

  // sorting O(n)

  void sort()
  {
    int32_t i {0};
    for ( int32_t current{head}; current != UNDEFINED; ++i )
    {
      swap(i, current);
      current = buffer[i].next;
    }
    head = 0;
    tail = i - 1;
    available = i < SIZE ? i : UNDEFINED;
    sorted = true;
  }

  // search O(log n)

  int32_t search(const T data) const
  {
    if ( !sorted )
      return -1;
    int32_t left {head};
    int32_t right {tail};
    while ( left < right )
    {
      int32_t middle {(right - left) / 2 + left};
      if ( data == buffer[middle].data )
	return middle;
      else if ( data < buffer[middle].data )
	right = middle;
      else
	left = middle + 1;
    }
    return left;
  }

  #ifdef CATCH_CONFIG_MAIN
  Buffer& begin()
  {
    iterator = head;
    return *this;
  }

  bool operator!=(const Buffer &rhs) const
  {
    return iterator != UNDEFINED;
  }

  const Buffer& end() const
  {
    return *this;
  }

  Buffer& operator++()
  {
    iterator = buffer[iterator].next;
    return *this;
  }

  const T& operator*() const
  {
    return buffer[iterator].data;
  }

  void doSwap(const int32_t i, const int32_t j)
  {
    swap(i, j);
  }

  int32_t getTail() const
  {
    return tail;
  }

  int32_t getAvailable() const
  {
    return available;
  }

  string dump() const
  {
    stringstream result;
    for (int i {0}; i < SIZE; i ++ )
      result << buffer[i].prev << ":" << buffer[i].next << ":" << buffer[i].data << " ";
    return result.str();
  }
  #endif
};
#endif
