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
  uint16_t measure;
  uint8_t beat;
  uint8_t ticks_per_beat;
  double bpm;
  uint8_t meter_n;
  uint8_t meter_d;
  uint32_t delay;
  Sequence &sequence;
  MIDIPort &midi_port;
  Timing &timing;

  void setBpm(double b)
  {
    bpm = b;
    delay = static_cast<uint32_t>(round(6e7 / (bpm * sequence.getTicks())));
  }

  void setMeter(uint8_t n, uint8_t d)
  {
    meter_n = n;
    meter_d = d;
    ticks_per_beat = 4 * sequence.getTicks() / d;
  }
public:
  Player(Sequence &s, Timing &t, MIDIPort &p)
    : position{0}, sequence{s}, timing{t}, midi_port{p}
  {
    setBpm(120.0);
    setMeter(4,4);
    measure = 0;
    beat = 0;
    ticks_per_beat = sequence.getTicks();
  }

  const double getBpm() const
  {
    return bpm;
  }

  const uint8_t getMeterN() const
  {
    return meter_n;
  }

  const uint8_t getMeterD() const
  {
    return meter_d;
  }

  const uint16_t getMeasure() const
  {
    return measure;
  }

  const uint8_t getBeat() const
  {
    return beat;
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
	case Event::Meter:
	  setMeter(event.param0, event.param1);
	  break;
	default:
	  midi_port.send(event);
      }
    }
    position += 1;
    if ( position % ticks_per_beat == 0 )
    {
      beat ++;
      if ( beat % meter_n == 0 )
      {
        beat = 0;
        measure ++;
      }
    }
    bool hasEvent {sequence.hasEvent()};

    uint32_t start {timing.getMicroseconds()};
    while ( timing.getMicroseconds() - start < delay )
      timing.delay(10);
    return hasEvent;
  }
};
#endif
