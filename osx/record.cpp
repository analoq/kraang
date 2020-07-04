#include <cmath>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include "../Sequence.hpp"
#include "../Player.hpp"
#include "MacMIDIPort.hpp"
#include "CTiming.hpp"

bool MidiInput {false};
MacMIDIPort midi_port{3, 1};
Sequence sequence;
Player player{sequence, midi_port};
uint8_t record_track {2};
uint16_t quantization {6};
int last_delay;

uint32_t quantize(const uint32_t position)
{
  uint32_t less {position / quantization * quantization};
  uint32_t more {less + quantization};
  if ( position - less <= more - position )
    return less;
  else
    return more;
}

static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
			void *srcConnRefCon)
{
  MacMIDIPort &port {*reinterpret_cast<MacMIDIPort *>(readProcRefCon)};
  const uint8_t *message {pktlist->packet[0].data};
  MidiInput = true;
  Event::Type type {static_cast<Event::Type>(message[0] & 0xF0)};
  switch ( type )
  {
    case Event::NoteOn:
      sequence.receiveEvent(2, Event{quantize(sequence.getTrack(record_track).position), type, 0, message[1], message[2]});
      break;
    case Event::NoteOff:
      sequence.receiveEvent(2, Event{sequence.getTrack(record_track).position, type, 0, message[1], message[2]});
      break;
    default:
      break;
  }
  port.send(sequence.getTrack(record_track).channel, Event{0, type, 0, message[1], message[2]});
}

void play_thread()
{
  for ( ;; )
  {
    auto start = chrono::high_resolution_clock::now();
    auto tick_time = chrono::microseconds(player.getDelay());
    player.tick();
    while ( chrono::high_resolution_clock::now() - start < tick_time )
      this_thread::sleep_for(chrono::microseconds(500));
  }
}

int main(int argc, char *argv[])
{
  CTiming timing;
  // metronome
  sequence.setTrackLength(1, 4);
  sequence.addEvent(1, Event{24*0, Event::NoteOn, 0, 60, 110});
  sequence.addEvent(1, Event{24*0 + 12, Event::NoteOff, 0, 60, 0});
  sequence.addEvent(1, Event{24*1, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*1 + 12, Event::NoteOff, 0, 60, 0});
  sequence.addEvent(1, Event{24*2, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*2 + 12, Event::NoteOff, 0, 60, 0});
  sequence.addEvent(1, Event{24*3, Event::NoteOn, 0, 60, 80});
  sequence.addEvent(1, Event{24*3 + 12, Event::NoteOff, 0, 60, 0});
  // record track test
  sequence.setTrackLength(record_track, 8);
  sequence.getTrack(record_track).record = true;
  sequence.getTrack(record_track).channel = 0;
  sequence.returnToZero();

  initscr();
  start_color();
  noecho(); // don't echo typed text
  raw(); // keypresses don't wait for EOL
  halfdelay(1); // block 100ms for a key
  curs_set(0); // make cursor invisible
  //init_pair(1, COLOR_WHITE, COLOR_BLUE);
  
  // launch play thread
  thread t(play_thread);

  bool done {false};
  while ( !done )
  {
    mvprintw(0, 1, "Ticks: %d", sequence.getTicks());
    mvprintw(1, 1, "Tempo: %f", player.getBpm() / 10.0);
    mvprintw(2, 1, "Quant: 1/%d", 4 * sequence.getTicks() / quantization);
    if ( MidiInput )
      mvprintw(0, 20, "MIDI: X");
    else
      mvprintw(0, 20, "MIDI: -");
    MidiInput = false;
    for ( uint8_t i{0}; i < TRACKS; i ++ )
    {
      const Track &track {sequence.getTrack(i)};
      mvprintw(4 + i, 0, "Trk %02d: C%02d L%02d P%03d", i, track.channel, track.length, track.position);
    }
    refresh();

    switch ( getch() )
    {
      case 'q':
	done = true;
	break;
    }
  }

  endwin();
}
