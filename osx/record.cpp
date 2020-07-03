#include <ncurses.h>
#include "../Sequence.hpp"
#include "../Player.hpp"
#include "MacMIDIPort.hpp"
#include "CTiming.hpp"

bool MidiInput {false};
Sequence sequence;

static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
			void *srcConnRefCon)
{
  //MacMIDIPort &port {*reinterpret_cast<MacMIDIPort *>(readProcRefCon)};
  const uint8_t *message {pktlist->packet[0].data};
  MidiInput = true;
  Event::Type type {static_cast<Event::Type>(message[0] & 0xF0)};
  switch ( type )
  {
    case Event::NoteOn:
    case Event::NoteOff:
      sequence.receiveEvent(2, Event{0, type, 0, message[1], message[2]});
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[])
{
  MacMIDIPort midi_port{1};
  CTiming timing;
  Player player{sequence, midi_port};
  // metronome
  sequence.setTrackLength(1, 4);
  sequence.addEvent(1, Event{24*0, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*0 + 12, Event::NoteOff, 0, 60, 80});
  sequence.addEvent(1, Event{24*1, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*1 + 12, Event::NoteOff, 0, 60, 80});
  sequence.addEvent(1, Event{24*2, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*2 + 12, Event::NoteOff, 0, 60, 80});
  sequence.addEvent(1, Event{24*3, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*3 + 12, Event::NoteOff, 0, 60, 80});
  // record track test
  sequence.setTrackLength(2, 4);
  sequence.getTrack(2).record = true;
  sequence.getTrack(2).channel = 0;
  sequence.returnToZero();

  initscr();
  start_color();
  noecho(); // don't echo typed text
  raw(); // keypresses don't wait for EOL
  halfdelay(1); // block 100ms for a key
  curs_set(0); // make cursor invisible
  //init_pair(1, COLOR_WHITE, COLOR_BLUE);
  
  bool done {false};
  while ( !done )
  {
    mvprintw(0, 1, "Ticks: %d", sequence.getTicks());
    mvprintw(1, 1, "Tempo: %f", player.getBpm() / 10.0);
    if ( MidiInput )
      mvprintw(0, 20, "MIDI: X");
    else
      mvprintw(0, 20, "MIDI: -");
    MidiInput = false;
    for ( uint8_t i{0}; i < TRACKS; i ++ )
    {
      const Track &track {sequence.getTrack(i)};
      mvprintw(3 + i, 0, "Trk %02d: C%02d L%02d P%03d", i, track.channel, track.length, track.position);
    }
    refresh();

    player.tick();

    if ( getch() == 'q' )
      done = true;
  }

  endwin();
}
