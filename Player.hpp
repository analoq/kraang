#ifndef PLAYER_HPP
#define PLAYER_HPP
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
  Sequence &sequence;
  MIDIPort &midi_port;
  Timing &timing;
public:
  Player(Sequence &s, Timing &t, MIDIPort &p)
    : position{0}, bpm{120.0}, sequence{s}, timing{t}, midi_port{p}
  {
  }

  void tick()
  {
    for ( ; sequence.hasEvent(); sequence.nextEvent())
    {
      const Event event = sequence.getEvent();
      if ( event.position > position )
	break;
      switch ( event.type )
      {
	case Event::Tempo:
	  bpm = event.getBpm();
	  break;
	default:
	  midi_port.send(event);
      }
    }
    timing.delay(6e7 / (bpm * sequence.getTicks()));
    position += 1;
  }
};
#endif
