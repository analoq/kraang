#ifdef CATCH_CONFIG_MAIN
#include <sstream>
using namespace std;
#endif

template<class T>
class Node
{
public:
  int32_t next;
  int32_t prev;
  T data;

  Node() : next{-1}, prev{-1}
  {
  }

  Node(T data) : data{data}, next{-1}, prev{-1}
  {
  }
};

template<class T, uint32_t SIZE>
class Buffer
{
private:
  int32_t available;
  int32_t head;
  int32_t tail;
  bool sorted;
  Node<T> buffer[SIZE];

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

    if ( buffer[i].next != -1 )
      buffer[buffer[i].next].prev = i;
    if ( buffer[j].prev != -1 )
      buffer[buffer[j].prev].next = j;
    if ( buffer[j].next != -1 )
      buffer[buffer[j].next].prev = j;
    if ( buffer[i].prev != -1 )
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
	buffer[i].next = -1;
      else
        buffer[i].next = i + 1;
    }
    tail = -1;
    available = 0;
    head = -1;
    sorted = false;
  }

  int32_t insert(const uint32_t index, const T data)
  {
    if ( index >= SIZE )
      return -1;
    if ( available == -1 )
      return -1;

    const int32_t curr_available {available};
    const int32_t next_available {buffer[available].next};
    if ( head == -1 && tail == -1 )
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

  int32_t insert(const T data)
  {
    return insert(available, data);
  }

  bool remove(uint32_t index)
  {
    if ( index >= SIZE )
      return false;

    if ( index == head && index == tail )
    {
      head = -1;
      tail = -1;
    }
    else if ( index == head )
    {
      head = buffer[head].next;
      buffer[head].prev = -1;
    }
    else if ( index == tail )
    {
      tail = buffer[tail].prev;
      buffer[tail].next = -1;
    }
    else if ( buffer[index].prev != -1 && buffer[index].next != -1 )
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

  void sort()
  {
    int32_t i {0};
    for ( int32_t current{head}; current != -1; ++i )
    {
      swap(i, current);
      current = buffer[i].next;
    }
    head = 0;
    tail = i - 1;
    available = i < SIZE ? i : -1;
    sorted = true;
  }

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
  void doSwap(const int32_t i, const int32_t j)
  {
    swap(i, j);
  }

  int32_t getHead() const
  {
    return head;
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

  string traverse() const
  {
    stringstream result;
    for ( int current{head}; current != -1; current = buffer[current].next )
      result << buffer[current].data;
    return result.str();
  }
  #endif
};
