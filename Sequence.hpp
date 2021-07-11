#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP
#include "Buffer.hpp"
#include "Event.hpp"

static const int TRACKS = 17;
static const int SIZE = 8192;
static const int TEMPO_TRACK = 0;

struct SeekResult
{
  int32_t position;
  uint32_t tempo;
  uint8_t numerator;
  uint8_t denominator;
};

struct Track
{
  int32_t position;
  uint8_t channel;
  uint8_t length;
  enum
  {
    OVERDUBBING,
    OVERWRITING,
    OFF,
    OFF_TO_OVERDUBBING,
    OFF_TO_OVERWRITING,
    OVERDUBBING_TO_OVERWRITING,
    TURNING_OFF,
  } state;

  Track() : position{0}, channel{0}, length{0}, state{OVERDUBBING}
  {
  }
};

class Sequence
{
private:
  Buffer<Event, SIZE, TRACKS> buffer;
  Track track[TRACKS];
  uint16_t ticks;

  const SeekResult getPosition(const uint16_t measure)
  {
    buffer.returnToZero(TEMPO_TRACK);
    uint16_t m {0};
    SeekResult result;
    result.position = 0;
    result.numerator = 4;
    result.denominator = 4;
    result.tempo = 500000;
    while ( buffer.notUndefined(TEMPO_TRACK) )
    {
      const Event &event {buffer.get(TEMPO_TRACK)};
      if ( event.getType() == Event::Meter )
      {
      	result.position = event.position;
      	m += result.position * result.denominator / (result.numerator * ticks * 4);
      	result.numerator = event.param0;
      	result.denominator = event.param1;
      	if ( m >= measure )
      	  break;
      }
      else if ( event.getType() == Event::Tempo )
      	result.tempo = event.getTempo();
      buffer.next(TEMPO_TRACK);
    }
    result.position += (measure - m)*result.numerator*ticks*4/result.denominator;
    return result;
  }

public:
  Sequence()
  {
    clear();
    track[0].channel = 0;
    for ( uint8_t i{1}; i < TRACKS; ++i )
      track[i].channel = i-1;
  }

  void clear()
  {
    buffer.clear();
    ticks = 24;
  }

  Track &getTrack(const uint8_t t)
  {
    return track[t];
  }

  void setTrackLength(const uint8_t t, const uint8_t l)
  {
    track[t].length = l;
  }

  uint16_t getTicks() const
  {
    return ticks;
  }

  void setTicks(uint16_t t)
  {
    ticks = t;
  }

  void returnToZero(const uint8_t t)
  {
    track[t].position = 0;
    buffer.returnToZero(t);
  }

  const SeekResult seek(const uint16_t measure)
  {
    SeekResult result {getPosition(measure)};
    for ( uint8_t t {0}; t < TRACKS; ++t )
    {
      buffer.seek(t, Event{result.position, Event::NoteOff, 0, 0, 0});
      if ( buffer.notUndefined(t) )
      {
	const Event &event {buffer.get(t)};
	if ( event.position < result.position )
	  buffer.setUndefined(t);
      }
    }
    return result;
  }

  Event& getEvent(uint8_t track)
  {
    return buffer.get(track);
  }

  bool notUndefined(const uint8_t track)
  {
    return buffer.notUndefined(track);
  }

  void nextEvent(const uint8_t track)
  {
    assert(buffer.notUndefined(track));
    buffer.next(track);
  }

  InsertResult<Event> addEvent(const uint8_t track, const Event &event)
  {
    return buffer.insert(track, event);
  }

  void removeEvent(const uint8_t track)
  {
    buffer.remove(track);
  }

  uint8_t getUsage() const
  {
    return (buffer.getCount() * 100) / SIZE;
  }

  #ifdef CATCH_CONFIG_MAIN
  Buffer<Event,SIZE,TRACKS> &getBuffer()
  {
    return buffer;
  }
  #endif
};
#endif
