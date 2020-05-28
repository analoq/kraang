#include "Sequence.hpp"

class File
{
public:
  virtual bool isValid() const = 0;

  virtual void close() = 0;

  virtual uint8_t readByte() = 0;

  virtual void read(uint32_t length, uint8_t *data) = 0;

  virtual uint32_t getPosition() const = 0;

  virtual void seek(int32_t position) = 0;
};


class MIDIFile
{
private:
  File &fp;

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
  MIDIFile(File &fp) : fp{fp}
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
    printf("Format: %d\n", format);
    printf("Tracks: %d\n", tracks);
    //printf("Ticks: %d\n", ticks);
    sequence.setTicks(ticks);
    // tracks
    for ( int16_t i = 0; i < tracks; i ++ )
    {
      sequence.returnToZero();
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
		    sequence.addEvent(Event{track_time, Event::NoteOff, channel, param1, 0});
		    break;
	    case Event::NoteOn:
		    param1 = fp.readByte();
		    param2 = fp.readByte();
		    if (param2)
		      sequence.addEvent(Event{track_time, Event::NoteOn, channel, param1, param2});
		    else
		      sequence.addEvent(Event{track_time, Event::NoteOff, channel, param1, 0});
		    break;
	    case Event::PolyAfter:
		    param1 = fp.readByte();
		    param2 = fp.readByte();
		    break;
	    case Event::Expression:
		    param1 = fp.readByte();
		    param2 = fp.readByte();
		    if ( param1 == 64)
		      printf("%d: Sustain %d\n", track_time, param1);
		    break;
	    case Event::ProgChange:
		    param1 = fp.readByte();
		    printf("%d: Program change %d\n", track_time, param1);
		    break;
	    case Event::AfterTouch:
		    param1 = fp.readByte();
		    break;
	    case Event::PitchBend:
		    param1 = fp.readByte();
		    param2 = fp.readByte();
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
				    sequence.setBpm(60000000.0 / read_int(3));
				    printf("%d: Tempo: %f\n", track_time, sequence.getBpm());
				    break;
				  case 0x58:
				    printf("%d: Time Signature: %d %d %d %d\n",
					     track_time, fp.readByte(), fp.readByte(), fp.readByte(), fp.readByte());
				    break;
				  case 0x03:
				    // track name
				    fp.read(size < sizeof(track_name) ? size : sizeof(track_name), track_name);
				    track_name[size] = 0;
				    printf("Track Name: %s\n", track_name);
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
