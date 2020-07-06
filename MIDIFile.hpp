#ifndef MIDIFILE_HPP
#define MIDIFILE_HPP
#include "Sequence.hpp"
#include "File.hpp"

class MIDIFile
{
private:
  KFile &fp;

  int32_t read_int(uint8_t length)
  {
    int32_t value = 0;
    while (length --)
      value = (value << 8) + fp.readByte();
    return value;
  }

  int32_t read_varlength()
  {
    int32_t value;
    uint8_t c;
    if ( (value = fp.readByte()) & 0x80 )
    {
      value &= 0x7f;
      do
      {
        value = (value << 7) + ((c = fp.readByte()) & 0x7f);
      } while (c & 0x80);
    }
    return value;
  }

public:
  MIDIFile(KFile &fp) : fp{fp}
  {
  }

  ~MIDIFile()
  {
    fp.close();
  }

  int8_t import(Sequence &sequence)
  {
    sequence.clear();
    // header
    fp.seek(8); // MThd, size
    int16_t format = read_int(2);
    int16_t tracks = read_int(2);
    int16_t ticks = read_int(2);
    sequence.setTicks(ticks);
    // tracks
    for ( int16_t i = 0; i < tracks; i ++ )
    {
	uint8_t track_name[80];
        uint32_t track_size, track_pos;
	// track header
	fp.seek(4); // MTrk
	track_size = read_int(4);
	track_pos = fp.getPosition();
	
	// events
	uint32_t track_time = 0;
	uint8_t status = 0;
	while (fp.getPosition() < track_pos+track_size)
	{
	  uint8_t event, channel;
	  uint8_t param1, param2;
	  
	  uint32_t delta_time = read_varlength();
	  track_time += delta_time;
	  
	  uint8_t peak_ahead = fp.readByte();
	  if (peak_ahead & 0x80)
	    status = peak_ahead;
	  else
	    fp.seek(-1);
	  
	  event = status & 0xF0;
	  channel = status & 0x0F;
	  
	  uint32_t size, type;
	  switch ( event )
	  {
	    case Event::NoteOff:
	      param1 = fp.readByte();
	      param2 = fp.readByte();
	      sequence.addEvent(channel+1, Event{track_time, Event::NoteOff, 0, param1, 0});
	      break;
	    case Event::NoteOn:
	      param1 = fp.readByte();
	      param2 = fp.readByte();
	      if (param2)
		sequence.addEvent(channel+1, Event{track_time, Event::NoteOn, 0, param1, param2});
	      else
		sequence.addEvent(channel+1, Event{track_time, Event::NoteOff, 0, param1, 0});
	      break;
	    case Event::PolyAfter:
	      param1 = fp.readByte();
	      param2 = fp.readByte();
	      break;
	    case Event::Expression:
	      param1 = fp.readByte();
	      param2 = fp.readByte();
	      switch ( param1 )
	      {
		case 0x01: // modulation
		case 0x07: // volume
		case 0x0A: // pan
		case 0x40: // sustain pedal
		  sequence.addEvent(channel+1, Event{track_time, Event::Expression, 0, param1, param2});
		  break;
	      }
	      break;
	    case Event::ProgChange:
	      param1 = fp.readByte();
	      sequence.addEvent(channel+1, Event{track_time, Event::ProgChange, 0, param1, 0});
	      break;
	    case Event::AfterTouch:
	      param1 = fp.readByte();
	      break;
	    case Event::PitchBend:
	      param1 = fp.readByte();
	      param2 = fp.readByte();
	      sequence.addEvent(channel+1, Event{track_time, Event::PitchBend, 0, param1, param2});
	      break;
	    case Event::SysEx:
	      switch ( channel )
	      {
		// SysEx Event
		case 0x0:
		case 0x7:
			break;
		// Meta Event
		case 0xF: 
		  type = fp.readByte();
		  size = read_varlength();
		  switch ( type )
		  {
		    case 0x51:
		      sequence.addEvent(TEMPO_TRACK, Event{track_time, Event::Tempo, fp.readByte(), fp.readByte(), fp.readByte()});
		      break;
		    case 0x58:
		      sequence.addEvent(TEMPO_TRACK, Event{track_time, Event::Meter, fp.readByte(),
					      static_cast<uint8_t>(1 << fp.readByte()), fp.readByte()});
		      fp.readByte(); // ignore # of 1/32nd notes per 24 MIDI clocks
		      break;
		    case 0x03:
		      // track name
		      fp.read(size < sizeof(track_name) ? size : sizeof(track_name), track_name);
		      track_name[size] = 0;
		      break;
			    
		    case 0x00:
			    // sequence number
		    case 0x01:
			    // text event
		    case 0x04:
			    // instrument name
		    case 0x06:
			    // marker
		    case 0x20:
			    // midi channel prefix
		    case 0x54:
			    // SMTPE offset
		    case 0x59:
			    // key signature
		    case 0x2f:
			    // end of track
		    default:
			    fp.seek(size);
	      }
	      break;
	    default:
	      return -3;
	  }
	  break;
	default:
	  return -4;
      }
    }
    }
    return 0;
  }
};
#endif
