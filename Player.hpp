#ifndef PLAYER_HPP
#define PLAYER_HPP
#include <math.h>
#include "Sequence.hpp"

class Timing
{
public:
  virtual uint32_t getMicroseconds() = 0;
  virtual void delay(uint32_t us) = 0;
};

class MIDIPort
{
public:
  MIDIPort()
  {
  }

  virtual void send(const Event &event) = 0;
};

class Player
{
private:
  uint32_t position;
  double bpm;
  uint32_t delay;
  Sequence &sequence;
  MIDIPort &midi_port;
  Timing &timing;

  void setBpm(double b)
  {
    bpm = b;
    delay = static_cast<uint32_t>(round(6e7 / (bpm * sequence.getTicks())));
  }
public:
  Player(Sequence &s, Timing &t, MIDIPort &p)
    : position{0}, sequence{s}, timing{t}, midi_port{p}
  {
    setBpm(120.0);
  }

  const double getBpm() const
  {
    return bpm;
  }

  const uint32_t getDelay() const
  {
    return delay;
  }

  bool tick()
  {
    for ( ; sequence.hasEvent(); sequence.nextEvent())
    {
      const Event event = sequence.getEvent();
      if ( event.position > position )
	break;
      switch ( event.type )
      {
	case Event::Tempo:
	  setBpm(event.getBpm());
	  break;
	default:
	  midi_port.send(event);
      }
    }
    position += 1;
    bool hasEvent {sequence.hasEvent()};

    uint32_t start {timing.getMicroseconds()};
    while ( timing.getMicroseconds() - start < delay )
      timing.delay(10);
    return hasEvent;
  }
};
#endif
