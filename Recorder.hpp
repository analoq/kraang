#ifndef RECORDER_HPP
#define RECORDER_HPP
#include "Player.hpp"
#include "Sequence.hpp"
#include "MIDIPort.hpp"

class Recorder
{
private:
  const Player &player;
  Sequence &sequence;
  MIDIPort &midi_port;
  uint8_t quantization;
  uint8_t record_track;
  const uint8_t metronome_track {0};
public:
  Recorder(const Player &p, Sequence &s, MIDIPort &mp)
    : player{p}, sequence{s}, midi_port{mp}, quantization{6}, record_track{0}
  {
    // create metronome track
    const uint32_t ticks {sequence.getTicks()};
    sequence.setTrackLength(metronome_track, 4);
    sequence.getTrack(metronome_track).channel = 0;
    sequence.addEvent(metronome_track, Event{ticks*0, Event::NoteOn, 0, 60, 110});
    sequence.addEvent(metronome_track, Event{ticks*0 + ticks/4, Event::NoteOff, 0, 60, 0});
    sequence.addEvent(metronome_track, Event{ticks*1, Event::NoteOn, 0, 60, 80});
    sequence.addEvent(metronome_track, Event{ticks*1 + ticks/4, Event::NoteOff, 0, 60, 0});
    sequence.addEvent(metronome_track, Event{ticks*2, Event::NoteOn, 0, 60, 80});
    sequence.addEvent(metronome_track, Event{ticks*2 + ticks/4, Event::NoteOff, 0, 60, 0});
    sequence.addEvent(metronome_track, Event{ticks*3, Event::NoteOn, 0, 60, 80});
    sequence.addEvent(metronome_track, Event{ticks*3 + ticks/4, Event::NoteOff, 0, 60, 0});
    // default remaining tracks to 2 bar lengths
    for ( uint8_t i{1}; i < TRACKS; ++i )
      sequence.setTrackLength(i, 8);
  }

  uint8_t getRecordTrack() const
  {
    return record_track;
  }

  void setRecordTrack(const uint8_t t)
  {
    record_track = t;
  }

  void toggleTrack(const uint8_t t)
  {
    Track &track {sequence.getTrack(t)};
    if ( track.state == Track::ON )
      track.state = Track::TURNING_OFF;
    else if ( track.state == Track::OFF )
    {
      track.state = Track::TURNING_ON;
      sequence.returnToZero(t);
    }
  }

  uint8_t getQuantization() const
  {
    return quantization;
  }

  uint32_t quantize(const uint32_t position) const
  {
    uint32_t less {position / quantization * quantization};
    uint32_t more {less + quantization};
    if ( position - less <= more - position )
      return less;
    else
      return more;
  }

  void receiveEvent(Event event)
  {
    const Track &track {sequence.getTrack(record_track)};
    event.position = track.position;
    if ( player.isPlaying() && (track.state == Track::ON || track.state == Track::TURNING_OFF) )
    {
      switch ( event.type )
      {
	case Event::NoteOn:
	  event.position = quantize(event.position);
	  if ( event.position > track.length*sequence.getTicks() )
	    event.position -= track.length*sequence.getTicks();
	  sequence.addEvent(record_track, event);
	  break;
	case Event::NoteOff:
	  event.param2 = 0;
	  sequence.addEvent(record_track, event);
	  break;
	default:
	  break;
      }
    }
    midi_port.send(track.channel, event);
  }
};
#endif
