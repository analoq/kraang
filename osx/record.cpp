#include <cmath>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include "../Sequence.hpp"
#include "../Player.hpp"
#include "../Recorder.hpp"
#include "MacMIDIPort.hpp"
#include "CTiming.hpp"

bool MidiInput {false};
MacMIDIPort midi_port{0, 1};
Sequence sequence;
Player player{sequence, midi_port};
Recorder recorder{player, sequence, midi_port};

static void MidiHandler(const MIDIPacketList *pktlist, void *readProcRefCon,
			void *srcConnRefCon)
{
  MacMIDIPort &port {*reinterpret_cast<MacMIDIPort *>(readProcRefCon)};
  const uint8_t *message {pktlist->packet[0].data};
  MidiInput = true;
  Event event{0, static_cast<Event::Type>(message[0] & 0xF0), 0, message[1], message[2]};
  recorder.receiveEvent(event);
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
  // record track test
  sequence.getTrack(1).channel = 1;
  recorder.setRecordTrack(1);

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
    mvprintw(2, 1, "Quant: 1/%d", 4 * sequence.getTicks() / recorder.getQuantization());
    mvprintw(3, 1, "Metro: %s", player.isMetronomeOn() ? "ON " : "OFF");
    mvprintw(0, 20, "RcrTrk: %d", recorder.getRecordTrack());
    if ( MidiInput )
      mvprintw(1, 20, "MIDIin: X");
    else
      mvprintw(1, 20, "MIDIin: -");
    mvprintw(2, 20, "Status: %s", player.isPlaying() ? "PLAY" : "STOP");
    MidiInput = false;
    for ( uint8_t i{0}; i < TRACKS; i ++ )
    {
      Track &track {sequence.getTrack(i)};
      char *state;
      switch ( track.state )
      {
	case Track::ON:
	  state = " ON ";
	  break;
	case Track::OFF:
	  state = " OFF";
	  break;
	case Track::TURNING_ON:
	  state = ">ON ";
	  break;
	case Track::TURNING_OFF:
	  state = ">OFF";
	  break;
	default:
	  state = "----";
      }
      mvprintw(5 + i, 0, "Trk %02d: [%c] C%02d L%02d P%03d S:%s",
		i, track.played ? 'X':' ', track.channel, track.length, track.position, state);
      track.played = false;
    }
    refresh();

    switch ( getch() )
    {
      case '1':
	recorder.setRecordTrack(1);
	break;
      case '2':
	recorder.setRecordTrack(2);
	break;
      case '3':
	recorder.setRecordTrack(3);
	break;
      case '4':
	recorder.setRecordTrack(4);
	break;
      case 'q':
	recorder.toggleTrack(1);
	break;
      case 'w':
	recorder.toggleTrack(2);
	break;
      case 'e':
	recorder.toggleTrack(3);
	break;
      case 'r':
	recorder.toggleTrack(4);
	break;
      case ' ':
	if ( player.isPlaying() )
	  player.stop();
	else
	  player.play();
	break;
      case 'm':
	player.setMetronome(!player.isMetronomeOn());
	break;
      case 'x':
	done = true;
	break;
    }
  }

  player.stop();
  endwin();
}
