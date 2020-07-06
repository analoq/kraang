#ifndef EVENT_HPP
#define EVENT_HPP
class Event
{
public:
  uint32_t position;
  enum Type : uint8_t
  {
    None = 0x00,
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
  } type;
  uint8_t param0;
  uint8_t param1;
  uint8_t param2;

  Event() : position{0}, type{None}, param0{0}, param1{0}, param2{0}
  {
  }

  Event(uint32_t p, Type t, uint8_t p0, uint8_t p1, uint8_t p2 )
    : position{p}, type{t}, param0{p0}, param1{p1}, param2{p2}
  {
  }

  const static Event allNotesOff()
  {
    return Event{0, Event::Expression, 0, 0x78, 0x00};
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
#endif
