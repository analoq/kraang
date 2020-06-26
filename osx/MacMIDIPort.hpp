#include "../Player.hpp"
#include <CoreMIDI/CoreMIDI.h>


class MacMIDIPort : public MIDIPort
{
private:
  MIDIClientRef MIDIClient;
  MIDIPortRef MIDIInPort;
  MIDIPortRef MIDIOutPort;
  MIDIEndpointRef MIDIDest;

  static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
                          void *srcConnRefCon)
  {
    const Byte *message = pktlist->packet[0].data;
  }

public:
  MacMIDIPort(const int destination)
  {
    MIDIClientCreate(CFSTR("analoq.kraang"), NULL, NULL, &MIDIClient);
    MIDIInputPortCreate(MIDIClient, CFSTR("Input port"), MidiHandler, this, &MIDIInPort);
    MIDIOutputPortCreate(MIDIClient, CFSTR("Output port"), &MIDIOutPort);
    //MIDISourceCreate(MIDIClient, CFSTR("kraang.output"), &MIDIOutput);
    MIDIDest = MIDIGetDestination(destination);
  }

  ~MacMIDIPort()
  {
    MIDIClientDispose(MIDIClient);
  }

  void send(uint8_t channel, const Event &event)
  {
    MIDIPacketList pktlist;
    MIDIPacket *packet = MIDIPacketListInit(&pktlist);
    unsigned char data[3];
    data[0] = event.type | channel;
    data[1] = event.param1;
    data[2] = event.param2;
    switch ( event.type )
    {
      case Event::ProgChange:
	MIDIPacketListAdd(&pktlist, sizeof(MIDIPacket), packet, 0, 2, data);
        break;
      default:
        MIDIPacketListAdd(&pktlist, sizeof(MIDIPacket), packet, 0, 3, data);
    }
    //MIDIReceived(MIDIOutput, &pktlist);
    MIDISend(MIDIOutPort, MIDIDest, &pktlist);
  }
};

