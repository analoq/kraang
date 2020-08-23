#ifndef MIDIPORT_HPP
#define MIDIPORT_HPP
#include "Event.hpp"

class MIDIPort
{
public:
  virtual void send(const uint8_t channel, const Event &event) = 0;
};

#endif
