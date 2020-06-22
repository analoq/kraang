#ifndef PLAYER_HPP
#define PLAYER_HPP
#include <math.h>
#include "Sequence.hpp"

class MIDIPort
{
public:
  virtual void send(const Event &event) = 0;
};

class Player
{
private:
  uint32_t position;
  uint16_t measure;
  uint8_t beat;
  uint16_t ticks_per_beat;
  uint8_t meter_n;
  uint8_t meter_d;
  uint32_t tempo;
  uint32_t delay;
  Sequence &sequence;
  MIDIPort &midi_port;
  bool events_remain;
  bool visuals_changed;

  void setTempo(uint32_t t)
  {
    tempo = t;
    delay = static_cast<uint32_t>(round(static_cast<double>(tempo) / sequence.getTicks()));
    visuals_changed = true;
  }

  void setMeter(uint8_t n, uint8_t d)
  {
    meter_n = n;
    meter_d = d;
    ticks_per_beat = 4 * sequence.getTicks() / d;
    visuals_changed = true;
  }
public:
  Player(Sequence &s, MIDIPort &p)
    : position{0}, sequence{s}, midi_port{p}
  {
    setTempo(500000);
    setMeter(4,4);
    returnToZero();
    visuals_changed = true;
  }

  const uint32_t getDelay() const
  {
    return delay;
  }

  const uint16_t getBpm() const
  {
    return round(600e6 / tempo);
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

  const bool visualsChanged()
  {
    if ( visuals_changed )
    {
      visuals_changed = false;
      return true;
    }
    return false;
  }

  void returnToZero()
  {
    position = 0;
    measure = 0;
    beat = 0;
    events_remain = true;
  }

  void seek(uint16_t m)
  {
    SeekResult result {sequence.seek(m)};
    position = result.position;
    measure = m;
    beat = 0;
    events_remain = true;
    setTempo(result.tempo);
    setMeter(result.numerator, result.denominator);
  }

  bool tick()
  {
    // tempo track
    if ( sequence.hasEvents(TEMPO_TRACK) )
    {
      while ( true )
      {
	const Event &event = sequence.getEvent(TEMPO_TRACK);
	if ( event.position > position )
	  break;
	switch ( event.type )
	{
	  case Event::Tempo:
	    setTempo(event.getTempo());
	    break;
	  case Event::Meter:
	    setMeter(event.param0, event.param1);
	    break;
	  default:
	    break;
	}
	if ( !sequence.nextEvent(TEMPO_TRACK) )
	  break;
      }
    }
 
    // music track
    if ( sequence.hasEvents(1) && events_remain )
    {
      while ( true )
      {
	const Event &event = sequence.getEvent(1);
	if ( event.position > position )
	  break;
	midi_port.send(event);
	if ( !sequence.nextEvent(1) )
	{
	  events_remain = false;
	  break;
	}
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
      visuals_changed = true;
    }

    return events_remain;
  }
};
#endif
