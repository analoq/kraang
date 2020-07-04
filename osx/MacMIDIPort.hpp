#include "../Player.hpp"
#include <CoreMIDI/CoreMIDI.h>
#include <iostream>

using namespace std;

static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
                        void *srcConnRefCon);
 
class MacMIDIPort : public MIDIPort
{
private:
  MIDIClientRef MIDIClient;
  MIDIPortRef MIDIInPort;
  MIDIPortRef MIDIOutPort;
  MIDIEndpointRef MIDIDest;
  MIDIEndpointRef MIDISource;

public:
  MacMIDIPort(const int out_port, const int in_port)
  {
    MIDIClientCreate(CFSTR("analoq.kraang"), NULL, NULL, &MIDIClient);
    MIDIInputPortCreate(MIDIClient, CFSTR("Input port"), MidiHandler, this, &MIDIInPort);
    MIDIOutputPortCreate(MIDIClient, CFSTR("Output port"), &MIDIOutPort);
    //MIDISourceCreate(MIDIClient, CFSTR("kraang.output"), &MIDIOutput);
    MIDIDest = MIDIGetDestination(out_port);
    MIDISource = MIDIGetSource(in_port);
    MIDIPortConnectSource(MIDIInPort, MIDISource, NULL);
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

