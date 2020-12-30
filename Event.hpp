#ifndef EVENT_HPP
#define EVENT_HPP
class Event
{
public:
  enum Type : uint8_t
  {
    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyAfter = 0xA0,
    Expression = 0xB0,
    ProgChange = 0xC0,
    AfterTouch = 0xD0,
    PitchBend = 0xE0,
    SysEx = 0xF0,
    Tempo = 0xFE,
    Meter = 0xFF,
  };
  int32_t position;
private:
	uint8_t type;
public:
  uint8_t param0;
  uint8_t param1;
  uint8_t param2;

  Event() : position{0}, type{NoteOff}, param0{0}, param1{0}, param2{0}
  {
  }

  Event(int32_t p, enum Type t, uint8_t p0, uint8_t p1, uint8_t p2 )
    : position{p}, type{t}, param0{p0}, param1{p1}, param2{p2}
  {
  }

  const static Event allNotesOff()
  {
    return Event{0, Event::Expression, 0, 0x78, 0x00};
  }

  const enum Type getType() const
  {
		return static_cast<enum Type>(type | 0x80);
  }

	void setNew(const bool is_new)
	{
		if ( is_new )
			type &= 0x7F;
		else
			type |= 0x80;
	}

	const bool isNew() const
	{
		return !(type & 0x80);
	}

  const uint32_t getTempo() const
  {
    return (param0 << 16 | param1 << 8 | param0);
  }

  bool operator >(const Event &a) const
  {
    return position > a.position;
  }

  bool operator >=(const Event &a) const
  {
    return position >= a.position;
  }

  bool operator <=(const Event &a) const
  {
    return position <= a.position;
  }
};

#ifdef CATCH_CONFIG_MAIN
const char *getNote(const int note)
{
  // 60 = C4
  const char *notes[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  return notes[note % 12];
}

int getOctave(const int note)
{
  return note / 12 - 1;
}

ostream& operator<<(ostream &o, const Event& event)
{ 
  o << event.position << ":";
  switch ( event.getType() )
  { 
    case Event::NoteOn:
      o << "NoteOn,"
        << getNote(event.param1) << getOctave(event.param1) << ","
        << static_cast<int>(event.param2);
      break;
    case Event::NoteOff:
      o << "NoteOff,"
        << getNote(event.param1) << getOctave(event.param1);
      break;
    case Event::Tempo: 
      o << "Tempo," << event.getTempo();
      break;
    case Event::Meter: 
      o << "Meter," << static_cast<int>(event.param0) << "/"
        << static_cast<int>(event.param1);
      break;
    default:
      o << static_cast<int>(event.getType())
        << "," << static_cast<int>(event.param1)
        << "," << static_cast<int>(event.param2);
  } 
  o << endl;
  return o;
}
#endif

#endif
