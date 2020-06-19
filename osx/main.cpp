#include "../Sequence.hpp"
#include "../Player.hpp"
#include "CFile.hpp"
#include "../MIDIFile.hpp"
#include <thread>
#include <iostream>
#include <iomanip>
#include <CoreMIDI/CoreMIDI.h>

using namespace std;

class CTiming
{
private:
  chrono::time_point<std::chrono::high_resolution_clock> start;
public:
  CTiming()
  {
    start = chrono::high_resolution_clock::now();
  }

  uint32_t getMicroseconds() const
  {
    auto now = chrono::high_resolution_clock::now();
    double diff = chrono::duration<double, std::micro>(now - start).count();
    return static_cast<uint32_t>(diff);
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
  MIDIEndpointRef MIDIDest;

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
    //MIDISourceCreate(MIDIClient, CFSTR("kraang.output"), &MIDIOutput);
    MIDIDest = MIDIGetDestination(1);
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

int main(int argc, char *argv[])
{
  if ( argc != 2 )
  {
    cout << "Usage: main <file.mid>" << endl;
    return -1;
  }
  MacMIDIPort midi_port;
  Sequence sequence;
  CTiming timing;
  CFile file{argv[1]};
  MIDIFile midi_file{file};
  midi_file.import(sequence);
  Player player{sequence, midi_port};
  cout << "Ticks: " << sequence.getTicks() << endl;
  cin.ignore(1);
  int count = 0;
  while (player.tick())
  {
    uint32_t start {timing.getMicroseconds()};
    if ( count++ % sequence.getTicks() == 0 )
    {
      cout << fixed << setw(4) << setprecision(1) << player.getBpm() << "\t"
           << fixed << setw(3) << player.getMeasure()+1 << ":"
  	   << fixed << setw(2) << static_cast<int>(player.getBeat()+1) << "\n";
    }
    while ( timing.getMicroseconds() - start < player.getDelay() )
      timing.delay(10);
  }
  return 0;
}
