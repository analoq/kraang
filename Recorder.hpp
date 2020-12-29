#ifndef RECORDER_HPP
#define RECORDER_HPP
#include "Sequence.hpp"
#include "MIDIPort.hpp"

class Recorder
{
private:
  Sequence &sequence;
  MIDIPort &midi_port;
  MIDIPort &metronome_port;
  uint8_t quantization;
  uint8_t record_track;
  const uint8_t metronome_track {0};
  bool is_playing;
  bool is_recording;
  bool metronome;
  static const uint8_t MAX_PENDING = 8;
  Event pending[MAX_PENDING];
  uint8_t pending_count = 0;
public:
  Recorder(Sequence &s, MIDIPort &mp, MIDIPort &metp)
    : is_playing{false}, is_recording{true}, metronome{true}, quantization{6}, record_track{1},
      sequence{s}, midi_port{mp}, metronome_port{metp}
  {
  }

  void initMetronome()
  {
    // create metronome track
    const uint32_t ticks {sequence.getTicks()};
    sequence.setTrackLength(metronome_track, 4);
    sequence.getTrack(metronome_track).channel = 0;
    sequence.addEvent(metronome_track, Event{ticks*0, Event::NoteOn, 0, 60, 110});
    sequence.addEvent(metronome_track, Event{ticks*0 + ticks/8, Event::NoteOff, 0, 60, 0});
    sequence.addEvent(metronome_track, Event{ticks*1, Event::NoteOn, 0, 60, 80});
    sequence.addEvent(metronome_track, Event{ticks*1 + ticks/8, Event::NoteOff, 0, 60, 0});
    sequence.addEvent(metronome_track, Event{ticks*2, Event::NoteOn, 0, 60, 80});
    sequence.addEvent(metronome_track, Event{ticks*2 + ticks/8, Event::NoteOff, 0, 60, 0});
    sequence.addEvent(metronome_track, Event{ticks*3, Event::NoteOn, 0, 60, 80});
    sequence.addEvent(metronome_track, Event{ticks*3 + ticks/8, Event::NoteOff, 0, 60, 0});
    // default remaining tracks to 2 bar lengths
    for ( uint8_t i{1}; i < TRACKS; ++i )
      sequence.setTrackLength(i, 8);
  }

  void setMetronome(bool m)
  {
    metronome = m;
    if ( !metronome )
    {
      const Track &track {sequence.getTrack(metronome_track)};
      metronome_port.send(track.channel, Event::allNotesOff());
    }
  }

  bool isMetronomeOn() const
  {
    return metronome;
  }

  void setIsPlaying(const bool playing)
  {
    is_playing = playing;
    if ( !is_playing )
    {
      const Track &track {sequence.getTrack(metronome_track)};
      metronome_port.send(track.channel, Event::allNotesOff());
    }
  }

  void setIsRecording(const bool recording)
  {
    is_recording = recording;
  }

  uint8_t getRecordTrack() const
  {
    return record_track;
  }

  void setRecordTrack(const uint8_t t)
  {
    record_track = t;
  }

  void toggleTrack(const uint8_t t, bool overwrite)
  {
    Track &track {sequence.getTrack(t)};
    switch ( track.state )
    {
      case Track::OFF:
	sequence.returnToZero(t);
	if ( overwrite )
	  track.state = Track::OFF_TO_OVERWRITING;
	else
	  track.state = Track::OFF_TO_OVERDUBBING;
	break;
      case Track::OVERDUBBING:
	if ( overwrite )
	  track.state = Track::OVERDUBBING_TO_OVERWRITING;
	else
	  track.state = Track::TURNING_OFF;
	break;
      case Track::OVERWRITING:
	track.state = Track::TURNING_OFF;
	break;
      case Track::OFF_TO_OVERWRITING:
      case Track::OFF_TO_OVERDUBBING:
	track.state = Track::OFF;
	break;
      case Track::OVERDUBBING_TO_OVERWRITING:
	track.state = Track::OVERDUBBING;
	break;
      case Track::TURNING_OFF:
	break;
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

  bool isRecordState(const uint8_t track_index, const Track &track)
  {
    return track_index == record_track && is_playing && is_recording &&
	 (track.state == Track::OVERDUBBING || 
	  track.state == Track::OVERWRITING ||
	  track.state == Track::TURNING_OFF);
  }

  void receiveEvent(Event event)
  {
    const Track &track {sequence.getTrack(record_track)};
    midi_port.send(track.channel, event);
    if ( isRecordState(record_track, track) )
    {
      assert(pending_count < MAX_PENDING);
      pending[pending_count++] = event;
    }
  }

  void handleTick(const uint8_t track_index)
  {
    const Track &track {sequence.getTrack(record_track)};
    if ( isRecordState(track_index, track) )
    {
      // insert all pending recorded events
      for ( uint8_t i = 0; i < pending_count; i ++ )
      {
	Event event = pending[i];
        event.position = track.position;
	if ( event.getType() == Event::NoteOn )
	{
	  event.position = quantize(event.position);
	  if ( event.position >= track.length*sequence.getTicks() )
	    event.position -= track.length*sequence.getTicks();
	  InsertResult<Event> insert_result = sequence.addEvent(record_track, event);
	  if ( insert_result.forward )
	    insert_result.new_node.setNew(true);
	}
	else if ( event.getType() == Event::NoteOff )
	{	
	  InsertResult<Event> insert_result = sequence.addEvent(record_track, event);
	  //if ( insert_result.forward )
	  //  insert_result.new_node.setNew(true);
	}
      }
      pending_count = 0;
    }
  }

  bool handlePlayEvent(const uint8_t track_index, Event &event, bool &advance_event)
  {
    const Track &track {sequence.getTrack(track_index)};
    advance_event = true;
    if ( track_index == 0 )
    {
      if ( event.getType() == Event::Tempo || event.getType() == Event::Meter )
        return false;
      if ( metronome )
        metronome_port.send(track.channel, event);
      return true;
    }
    else if ( is_recording && track_index == record_track )
    {
      if ( track.state == Track::OVERDUBBING )
      {
	if ( event.isNew() )
	{
	  event.setNew(false);
	  return true;
	}
      }
      else if ( track.state == Track::OVERWRITING )
      {
	if ( event.isNew() )
	  event.setNew(false);
	else
	{
          sequence.removeEvent(record_track);
          advance_event = false;
	}
        return true;
      }
    }
    return false;
  }

  void handleMeasure()
  {
    // flip tracks on or off
    for ( uint8_t i{1}; i < TRACKS; ++i )
    {
	Track &track {sequence.getTrack(i)};
	switch ( track.state )
	{
	  case Track::OFF_TO_OVERDUBBING:
	    track.state = Track::OVERDUBBING;
	    break;
      	  case Track::OFF_TO_OVERWRITING:
	    track.state = Track::OVERWRITING;
	    break;
	  case Track::TURNING_OFF:
	    track.state = Track::OFF;
	    break;
	  case Track::OVERDUBBING:
	  case Track::OVERDUBBING_TO_OVERWRITING:
	  case Track::OVERWRITING:
	  case Track::OFF:
	    break;
	}
    }
  }

  void handleLoopEnd(Track &track)
  {
    switch ( track.state )
    {
      case Track::OVERDUBBING_TO_OVERWRITING:
	track.state = Track::OVERWRITING;
	break;
      case Track::OVERWRITING:
	track.state = Track::OVERDUBBING;
	break;
      case Track::OVERDUBBING:
      case Track::OFF_TO_OVERDUBBING:
      case Track::OFF_TO_OVERWRITING:
      case Track::TURNING_OFF:
      case Track::OFF:
	break;
    }
  }
};
#endif
