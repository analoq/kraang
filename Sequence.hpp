#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP
#include "Buffer.hpp"

class Event
{
public:
  uint32_t position;
  enum Type : uint8_t
  {
    None = 0x00,
    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyAfter = 0xA0,
    Expression = 0xB0,
    ProgChange = 0xC0,
    AfterTouch = 0xD0,
    PitchBend = 0xE0,
    SysEx = 0xF0,
    Tempo = 0xFE,
    Meter = 0xFF,
  } type;
  union
  {
    uint8_t channel;
    uint8_t param0;
  };
  uint8_t param1;
  uint8_t param2;

  Event() : position{0}, type{None}, param0{0}, param1{0}, param2{0}
  {
  }

  Event(uint32_t p, Type t, uint8_t p0, uint8_t p1, uint8_t p2 )
    : position{p}, type{t}, param0{p0}, param1{p1}, param2{p2}
  {
  }

  const double getBpm() const
  {
    return 60e6 / (param0 << 16 | param1 << 8 | param0);
  }

  bool operator <(const Event &a) const
  {
    return position < a.position;
  }

  bool operator ==(const Event &a) const
  {
    return position == a.position;
  }
};

static const int SIZE = 2048;

class Sequence
{
private:
  Buffer<Event, SIZE> buffer;
  uint16_t ticks;
  int16_t index;
public:
  Sequence()
  {
    clear();
  }

  void clear()
  {
    buffer.clear();
    index = UNDEFINED;
    ticks = 24;
  }

  uint16_t getTicks() const
  {
    return ticks;
  }

  void setTicks(uint16_t t)
  {
    ticks = t;
  }

  void returnToHead()
  {
    index = buffer.getHead();
  }

  void returnToZero()
  {
    index = UNDEFINED;
  }

  const bool hasEvent() const
  {
    return index != UNDEFINED;
  }

  const Event& getEvent() const
  {
    assert(index >= 0 && index < SIZE);
    return buffer[index];
  }

  void nextEvent()
  {
    if ( index != UNDEFINED )
      index = buffer.getNext(index);
  }

  void addEvent(Event event)
  {
    if ( index == UNDEFINED )
      index = buffer.insert_sorted(event);
    else
      index = buffer.insert_sorted(index, event);
  }

  #ifdef CATCH_CONFIG_MAIN
  Buffer<Event,SIZE> &getBuffer()
  {
    return buffer;
  }
  #endif
};
#endif
