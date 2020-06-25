#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP
#include "Buffer.hpp"
#include "Event.hpp"

static const int TRACKS = 17;
static const int SIZE = 16384;
static const int TEMPO_TRACK = 0;

struct SeekResult
{
  uint32_t position;
  uint32_t tempo;
  uint8_t numerator;
  uint8_t denominator;
};

struct Track
{
  uint8_t channel;
  uint8_t length;

  Track() : channel{0}, length{0}
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
      if ( event.type == Event::Meter )
      {
      	result.position = event.position;
      	m += result.position * result.denominator / (result.numerator * ticks * 4);
      	result.numerator = event.param0;
      	result.denominator = event.param1;
      	if ( m >= measure )
      	  break;
      }
      else if ( event.type == Event::Tempo )
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

  const Track &getTrack(uint8_t t) const
  {
    return track[t];
  }

  uint16_t getTicks() const
  {
    return ticks;
  }

  void setTicks(uint16_t t)
  {
    ticks = t;
  }

  void returnToZero()
  {
    for ( uint8_t track {0}; track < TRACKS; ++track )
      buffer.returnToZero(track);
  }

  const SeekResult seek(const uint16_t measure, bool events_remain[TRACKS])
  {
    SeekResult result {getPosition(measure)};
    for ( uint8_t track {0}; track < TRACKS; ++track )
    {
      buffer.seek(track, Event{result.position, Event::None, 0, 0, 0});
      if ( buffer.notUndefined(track) )
      {
	const Event &event {buffer.get(track)};
	events_remain[track] = event.position >= result.position;
      }
      else
	events_remain[track] = false;
    }
    return result;
  }

  const Event& getEvent(uint8_t track) const
  {
    return buffer.get(track);
  }

  bool hasEvents(const uint8_t track)
  {
    return buffer.hasEvents(track);
  }

  bool nextEvent(const uint8_t track)
  {
    if ( buffer.hasNext(track) )
    {
      buffer.next(track);
      return true;
    }
    return false;
  }

  void addEvent(const uint8_t track, const Event event)
  {
    buffer.insert(track, event);
  }

  #ifdef CATCH_CONFIG_MAIN
  Buffer<Event,SIZE,TRACKS> &getBuffer()
  {
    return buffer;
  }
  #endif
};
#endif
