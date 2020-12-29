#ifndef PLAYER_HPP
#define PLAYER_HPP
#include <math.h>
#include "Sequence.hpp"
#include "MIDIPort.hpp"
#include "Recorder.hpp"

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
  Recorder &recorder;
  bool playing;
  bool visuals_changed;

public:
  Player(Sequence &s, MIDIPort &p, Recorder &r)
    : position{0}, sequence{s}, midi_port{p}, recorder{r}, playing{false}
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
    recorder.setIsPlaying(playing);
  }

  void stop()
  {
    playing = false;
    recorder.setIsPlaying(playing);
    for ( uint8_t c {0}; c < 16; ++c )
      midi_port.send(c, Event::allNotesOff());
  }

  bool isPlaying() const
  {
    return playing;
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

  bool tick(const bool send_events = true)
  {
    if ( !playing )
      return true;
    for ( uint8_t i{0}; i < TRACKS; ++i )
    {
      Track &track {sequence.getTrack(i)};

      if ( track.state == Track::OVERDUBBING ||
           track.state == Track::OVERWRITING ||
	   track.state == Track::OVERDUBBING_TO_OVERWRITING ||
	   track.state == Track::TURNING_OFF )
      {
      	if ( send_events )
      	{
	  while ( sequence.notUndefined(i) )
	  {
	      Event &event = sequence.getEvent(i);
	      if ( event.position > track.position )
		  break;
	      bool advance_event;
	      if ( !recorder.handlePlayEvent(i, event, advance_event) )
	      {
		switch ( event.getType() )
		{
		    case Event::Tempo:
			setTempo(event.getTempo());
			break;
		    case Event::Meter:
			setMeter(event.param0, event.param1);
			break;
		    default:
			midi_port.send(track.channel, event);
		}
	      }

	      if ( advance_event )
		sequence.nextEvent(i);
	  }
      	}

				recorder.handleTick(i);
        track.position ++;
        if ( track.length && track.position == track.length*sequence.getTicks() )
        {
            track.position = 0;
            sequence.returnToZero(i);
						recorder.handleLoopEnd(track);
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
	recorder.handleMeasure();
     }
      visuals_changed = true;
    }

    for ( uint8_t i{1}; i < TRACKS; ++i )
      if ( sequence.notUndefined(i) )
        return true;
    return false;
  }
};
#endif
