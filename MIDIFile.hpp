#include "Sequence.hpp"

class MIDIFile
{
private:
  FILE *fp;

  int32_t read_int(uint8_t length)
  {
    int32_t value = 0;
    while (length --)
      value = (value << 8) + fgetc(fp);
    return value;
  }

  int32_t read_varlength()
  {
    int32_t value;
    uint8_t c;
    if ( (value = fgetc(fp)) & 0x80 )
    {
      value &= 0x7f;
      do
      {
        value = (value << 7) + ((c = getc(fp)) & 0x7f);
      } while (c & 0x80);
    }
    return value;
  }

public:
  MIDIFile(FILE *fp) : fp{fp}
  {
  }

  ~MIDIFile()
  {
    fclose(fp);
  }

  int8_t import(Sequence &sequence)
  {
    sequence.clear();
    // header
    fseek(fp, 8, SEEK_CUR); // MThd, size
    int16_t format = read_int(2);
    int16_t tracks = read_int(2);
    int16_t ticks = read_int(2);
    //printf("Format: %d\n", format);
    //printf("Tracks: %d\n", tracks);
    //printf("Ticks: %d\n", ticks);
    sequence.setTicks(ticks);
    // tracks
    for ( int16_t i = 0; i < tracks; i ++ )
    {
      sequence.returnToZero();
	uint8_t track_name[80];
        uint32_t track_size, track_pos;
	// track header
	fseek(fp, 4, SEEK_CUR); // MTrk
	track_size = read_int(4);
	track_pos = ftell(fp);
	
	// events
	uint32_t track_time = 0;
	uint8_t status = 0;
	while (ftell(fp) < track_pos+track_size)
	{
	  uint8_t event, channel;
	  uint8_t param1, param2;
	  
	  uint32_t delta_time = read_varlength();
	  track_time += delta_time;
	  
	  uint8_t peak_ahead = fgetc(fp);
	  if (peak_ahead & 0x80)
	    status = peak_ahead;
	  else
	    fseek(fp, -1, SEEK_CUR);
	  
	  event = status & 0xF0;
	  channel = status & 0x0F;
	  
	  uint32_t size, type;
	  switch ( event )
	  {
	    case Event::NoteOff:
		    param1 = fgetc(fp);
		    param2 = fgetc(fp);
		    sequence.addEvent(Event{track_time, Event::NoteOff, channel, param1, 0});
		    break;
	    case Event::NoteOn:
		    param1 = fgetc(fp);
		    param2 = fgetc(fp);
		    if (param2)
		      sequence.addEvent(Event{track_time, Event::NoteOn, channel, param1, param2});
		    else
		      sequence.addEvent(Event{track_time, Event::NoteOff, channel, param1, 0});
		    break;
	    case Event::PolyAfter:
		    param1 = fgetc(fp);
		    param2 = fgetc(fp);
		    break;
	    case Event::Expression:
		    param1 = fgetc(fp);
		    param2 = fgetc(fp);
		    //if ( param1 == 64)
		    //  printf("Sustain %d\n", param1);
		    break;
	    case Event::ProgChange:
		    param1 = fgetc(fp);
		    //printf("Program change %d\n", param1);
		    break;
	    case Event::AfterTouch:
		    param1 = fgetc(fp);
		    break;
	    case Event::PitchBend:
		    param1 = fgetc(fp);
		    param2 = fgetc(fp);
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
			  type = fgetc(fp);
			  size = read_varlength();
			  switch ( type )
			  {
				  case 0x51:
				    //printf("Tempo: %f\n", 60000000.0 / read_int(3));
				    sequence.setBpm(60000000.0 / read_int(3));
				    break;
				  case 0x58:
				    printf("Time Signature: %d %d %d %d\n",
					     fgetc(fp), fgetc(fp), fgetc(fp), fgetc(fp));
				    break;
				  case 0x03:
				    // track name
				    fread(track_name, size < sizeof(track_name) ? size : sizeof(track_name), 1, fp);
				    track_name[size] = 0;
				    //printf("Track Name: %s\n", track_name);
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
					  fseek(fp, size, SEEK_CUR);
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
