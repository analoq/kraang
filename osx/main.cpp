#include "../Sequence.hpp"
#include "../Player.hpp"
#include "CFile.hpp"
#include "../MIDIFile.hpp"
#include <thread>
#include <iostream>
#include <CoreMIDI/CoreMIDI.h>

using namespace std;

class CTiming : public Timing
{
public:
  uint32_t getMicroseconds()
  {
    return 0;
  }
 
  void delay(uint32_t us)
  {
    this_thread::sleep_for(chrono::microseconds{us});
  }
};

class MacMIDIPort : public MIDIPort
{
private:
  MIDIClientRef MIDIClient;
  MIDIPortRef MIDIInPort;
  MIDIPortRef MIDIOutPort;
  MIDIEndpointRef MIDIOutput;

  static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
                          void *srcConnRefCon)
  {
    const Byte *message = pktlist->packet[0].data;
  }

public:
  MacMIDIPort()
  {
    MIDIClientCreate(CFSTR("analoq.kraang"), NULL, NULL, &MIDIClient);
    MIDIInputPortCreate(MIDIClient, CFSTR("Input port"), MidiHandler, this, &MIDIInPort);
    MIDIOutputPortCreate(MIDIClient, CFSTR("Output port"), &MIDIOutPort);
    MIDISourceCreate(MIDIClient, CFSTR("kraang.output"), &MIDIOutput);
  }

  ~MacMIDIPort()
  {
    MIDIClientDispose(MIDIClient);
  }

  void send(const Event &event)
  {
    MIDIPacketList pktlist;
    MIDIPacket *packet = MIDIPacketListInit(&pktlist);
    unsigned char data[3];
    data[0] = event.type | event.channel;
    data[1] = event.param1;
    data[2] = event.param2;
    MIDIPacketListAdd(&pktlist, sizeof(MIDIPacket), packet, 0, 3, data);
    MIDIReceived(MIDIOutput, &pktlist);
    //MIDISend(MIDIOutPort, MIDIOutPort, &pktlist);
  }
};

int main(int argc, char *argv[])
{
  MacMIDIPort midi_port;
  Sequence sequence;
  CTiming timing;
  CFile file{"midi_0.mid"};
  MIDIFile midi_file{file};
  midi_file.import(sequence);
  Player player{sequence, timing, midi_port};
  cin.ignore(1);
  for ( int i{0}; i < 480*4; i ++ )
    player.tick();
  return 0;
}
