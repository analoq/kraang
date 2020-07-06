#ifndef PLAYER_HPP
#define PLAYER_HPP
#include <math.h>
#include "Sequence.hpp"
#include "MIDIPort.hpp"

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
  bool playing;
  bool metronome;
  bool visuals_changed;

public:
  Player(Sequence &s, MIDIPort &p)
    : position{0}, sequence{s}, midi_port{p}, playing{false}, metronome{true}
  {
    setTempo(500000);
    setMeter(4,4);
    returnToZero();
    visuals_changed = true;
  }

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

  void play()
  {
    returnToZero();
    playing = true;
  }

  void stop()
  {
    playing = false;
  }

  bool isPlaying() const
  {
    return playing;
  }

  void setMetronome(bool m)
  {
    metronome = m;
  }

  bool isMetronomeOn() const
  {
    return metronome;
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
    for ( uint8_t i{0}; i < TRACKS; ++i )
      sequence.returnToZero(i);
  }

  void seek(uint16_t m)
  {
    SeekResult result {sequence.seek(m)};
    position = result.position;
    for ( uint8_t i{0}; i < TRACKS; ++i )
    {
      Track &track {sequence.getTrack(i)};
      track.position = position;
    }
    measure = m;
    beat = 0;
    setTempo(result.tempo);
    setMeter(result.numerator, result.denominator);
  }

  bool tick()
  {
    if ( !playing )
      return true;
    for ( uint8_t i{0}; i < TRACKS; ++i )
    {
      Track &track {sequence.getTrack(i)};

      if ( track.state == Track::ON || track.state == Track::TURNING_OFF )
      {
	if ( sequence.hasEvents(i) && track.events_remain )
	{
	  while ( true )
	  {
	    const Event &event = sequence.getEvent(i);
	    if ( event.position > track.position )
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
		if ( i != 0 || metronome )
		{
		  track.played = true;
		  midi_port.send(track.channel, event);
		}
	    }
	    if ( !sequence.nextEvent(i) )
	    {
	      track.events_remain = false;
	      break;
	    }
	  }
	}
	else
	  track.events_remain = false;

	track.position ++;
	if ( track.length && track.position == track.length*sequence.getTicks() )
	{
	  track.position = 0;
	  track.events_remain = true;
	  sequence.returnToZero(i);
	}
      }
    }

    position ++;
    if ( position % ticks_per_beat == 0 )
    {
      beat ++;
      if ( beat % meter_n == 0 )
      {
        beat = 0;
        measure ++;
	// flip tracks on or off
	for ( uint8_t i{1}; i < TRACKS; ++i )
	{
	  Track &track {sequence.getTrack(i)};
	  if ( track.state == Track::TURNING_ON )
	    track.state = Track::ON;
	  else if ( track.state == Track::TURNING_OFF )
	    track.state = Track::OFF;
	}
      }
      visuals_changed = true;
    }

    for ( uint8_t i{1}; i < TRACKS; ++i )
      if ( sequence.getTrack(i).events_remain )
        return true;
    return false;
  }
};
#endif
