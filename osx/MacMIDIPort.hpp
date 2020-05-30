#include "../Player.hpp"
#include <CoreMIDI/CoreMIDI.h>

class MacMIDIPort : public MIDIPort
{
private:
  MIDIClientRef MIDIClient;
  MIDIPortRef MIDIInPort;
  MIDIPortRef MIDIOutPort;

  static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
                          void *srcConnRefCon)
  {
    const Byte *message = pktlist->packet[0].data;
  }

public:
  void MidiInit()
  {
    MIDIClientCreate(CFSTR("analoq.sequencr"), NULL, NULL, &MIDIClient);
    MIDIInputPortCreate(MIDIClient, CFSTR("Input port"), MidiHandler, this, &MIDIInPort);
    MIDIOutputPortCreate(MIDIClient, CFSTR("Output port"), &MIDIOutPort);
  }

  void MIDISendEvent(struct Event *event, MIDIEndpointRef dest)
  {
    MIDIPacketList pktlist;
    MIDIPacket *packet = MIDIPacketListInit(&pktlist);
    unsigned char data[3];
    MIDIPacketListAdd(&pktlist, sizeof(MIDIPacket), packet, 0, 3, data);
    MIDISend(MIDIOutPort, dest, &pktlist);
  }
};
