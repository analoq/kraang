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
  } type;
  uint8_t channel;
  uint8_t param1;
  uint8_t param2;

  Event() : position{0}, type{None}, channel{0}, param1{0}, param2{0}
  {
  }

  Event(uint32_t p, enum Type t, uint8_t c, uint8_t p1, uint8_t p2 )
    : position{p}, type{t}, channel{c}, param1{p1}, param2{p2}
  {
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

static const int SIZE = 1024;

class Sequence
{
private:
  Buffer<Event, SIZE> buffer;
  double bpm;
  uint16_t ticks;
  int32_t index;
public:
  Sequence()
  {
    clear();
  }

  void clear()
  {
    buffer.clear();
    index = -1;
    bpm = 120.0;
    ticks = 24;
  }

  double getBpm() const
  {
    return bpm;
  }

  void setBpm(double b)
  {
    bpm = b;
  }

  uint16_t getTicks() const
  {
    return ticks;
  }

  void setTicks(uint16_t t)
  {
    ticks = t;
  }

  void returnToZero()
  {
    index = -1;
  }

  void addEvent(Event event)
  {
    if ( index == -1 )
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
