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
private:
  chrono::time_point<std::chrono::high_resolution_clock> start;
public:
  CTiming()
  {
    start = chrono::high_resolution_clock::now();
  }

  uint32_t getMicroseconds()
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
  Player player{sequence, timing, midi_port};
  cout << "Ticks: " << sequence.getTicks() << endl;
  cin.ignore(1);
  while (player.tick())
    cout << player.getBpm() << "\t" << player.getDelay() << " \r";
  return 0;
}
