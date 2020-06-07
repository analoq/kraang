#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP
#include "Buffer.hpp"
#include "Event.hpp"

static const int TRACKS = 2;
static const int SIZE = 2048;

class Sequence
{
private:
  Buffer<Event, SIZE, TRACKS> buffer;
  uint16_t ticks;
public:
  Sequence()
  {
    clear();
  }

  void clear()
  {
    buffer.clear();
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

  void returnToZero()
  {
    for ( uint8_t track {0}; track < TRACKS; ++track )
      buffer.returnToZero(track);
  }

  const bool hasEvent(uint8_t track) const
  {
    return buffer.notUndefined(track);
  }

  const Event& getEvent(uint8_t track) const
  {
    return buffer.get(track);
  }

  void nextEvent(uint8_t track)
  {
    buffer.next(track);
  }

  void addEvent(const uint8_t track, const Event event)
  {
    buffer.insert(track, event);
  }

  #ifdef CATCH_CONFIG_MAIN
  Buffer<Event,SIZE,TRACKS> &getBuffer()
  {
    return buffer;
  }
  #endif
};
#endif
