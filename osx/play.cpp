#include "../Sequence.hpp"
#include "../Player.hpp"
#include "CFile.hpp"
#include "CTiming.hpp"
#include "MacMIDIPort.hpp"
#include "../MIDIFile.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
			void *srcConnRefCon)
{
  cout << "Input!" << endl;
}

int main(int argc, char *argv[])
{
  if ( argc != 2 )
  {
    cout << "Usage: main <file.mid>" << endl;
    return -1;
  }
  MacMIDIPort midi_port{3, 0};
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
