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
Recorder recorder{sequence, midi_port};
Player player{sequence, midi_port, recorder};

static void MidiHandler(const MIDIPacketList *packetList, void *readProcRefCon,
			void *srcConnRefCon)
{
  //MacMIDIPort &port {*reinterpret_cast<MacMIDIPort *>(readProcRefCon)};
  const MIDIPacket *packet = packetList->packet;
  uint8_t lastStatus {0x00};
  for (int i = 0; i < packetList->numPackets; ++i)
  {
    const uint8_t *message {packet->data};
    for ( int j = 0; j < packet->length; )
    {
      uint8_t status {message[j++]};
      uint8_t param1;
      uint8_t param2;
      if ( (message[0] & 0x80) == 0 )
      {
	param1 = status;
	status = lastStatus;
      }
      else
      {
	param1 = message[j++];
	lastStatus = status;
      }
      Event::Type type {static_cast<Event::Type>(status & 0xF0)};
      param2 = message[j++];
      if ( type == Event::NoteOn && param2 == 0 )
	type = Event::NoteOff;
      recorder.receiveEvent(Event{0, type, 0, param1, param2});
    }
    packet = MIDIPacketNext(packet);
  }
  MidiInput = true;
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

enum {
  NORMAL,
  SELECT_TRACK,
} Mode;

enum {
  OFF,
  OVERDUB,
} RecMode;

bool RecReplace = false;

void handle_track(const uint8_t t)
{
  if ( Mode == SELECT_TRACK )
  {
    recorder.setRecordTrack(t);
    Mode = NORMAL;
  }
  else
    recorder.toggleTrack(t);
}

int main(int argc, char *argv[])
{
  CTiming timing;
  // metronome
  recorder.initMetronome();
  
  // record track test
  sequence.getTrack(1).channel = 1;
  sequence.getTrack(2).channel = 2;
  sequence.getTrack(3).channel = 3;
  sequence.getTrack(4).channel = 4;

  // turn tracks off
  for ( int i = 1; i < TRACKS; i ++ )
    sequence.getTrack(i).state = Track::OFF;

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
  Mode = NORMAL;
  while ( !done )
  {
    mvprintw(0, 20, "MIDIin: [%c]", MidiInput ? 'X' : ' ');
    mvprintw(1, 20, "MIDIout:[%c]", midi_port.played ? 'X' : ' ');

    mvprintw(0, 0, "T%03d Q%02d S%02d L%02d",
		    player.getBpm() / 10,
		    4 * sequence.getTicks() / recorder.getQuantization(),
		    50,
		    sequence.getTrack(0).length);

    Track &track {sequence.getTrack(recorder.getRecordTrack())};
    mvprintw(1, 0, "t%02d c%02d l%02d p%03d",
	     recorder.getRecordTrack(), track.channel, track.length, track.position);
 
    mvprintw(3, 0, "[%c]", player.isMetronomeOn() ? 'X' : ' ');
    mvprintw(4, 0, "Metro");

    mvprintw(3, 8, "[%c]", player.isPlaying() ? 'X' : ' ');
    mvprintw(4, 8, "Play");

    mvprintw(3, 16, "[%c]", Mode == SELECT_TRACK ? 'X' : ' ');
    mvprintw(4, 16, "Select");

    if ( RecMode == OFF )
      mvprintw(3, 24, "[ ]");
    else if ( RecMode == OVERDUB )
      mvprintw(3, 24, "[X]");
    mvprintw(4, 24, "Replace");

    MidiInput = false;
    for ( uint8_t i{0}; i < 3; i ++ )
    {
      for ( uint8_t j{0}; j < 4; j ++ )
      {
	const uint8_t track_index = i*4+j+1;
	Track &track {sequence.getTrack(track_index)};
	char state;
	switch ( track.state )
	{
	  case Track::ON:
	    state = 'X';
	    break;
	  case Track::OFF:
	    state = ' ';
	    break;
	  case Track::TURNING_ON:
	    state = '<';
	    break;
	  case Track::TURNING_OFF:
	    state = '>';
	    break;
	  default:
	    state = '-';
	}
	mvprintw(6+i*3, j*8, "[%c]", state);
	mvprintw(7+i*3, j*8, "%02d", track_index);
      }
    }
    refresh();
    midi_port.played = false;

    switch ( getch() )
    {
      case '1':
	player.setMetronome(!player.isMetronomeOn());
	break;
      case '2':
	if ( player.isPlaying() )
	  player.stop();
	else
	  player.play();
	break;
      case '3':
	if ( Mode == NORMAL )
	  Mode = SELECT_TRACK;
	else
	  Mode = NORMAL;
	break;
      case '4':
	if ( RecMode == OFF )
	  RecMode = OVERDUB;
	else if ( RecMode == OVERDUB )
	  RecMode = OFF;
	break;
      case 'q':
	handle_track(1);
	break;
      case 'w':
	handle_track(2);
	break;
      case 'e':
	handle_track(3);
	break;
      case 'r':
	handle_track(4);
	break;
      case 'a':
	handle_track(5);
	break;
      case 's':
	handle_track(6);
	break;
      case 'd':
	handle_track(7);
	break;
      case 'f':
	handle_track(8);
	break;
      case 'z':
	handle_track(9);
	break;
      case 'x':
	handle_track(10);
	break;
      case 'c':
	handle_track(11);
	break;
      case 'v':
	handle_track(12);
	break;
    case ' ':
	done = true;
	break;
    }
  }

  player.stop();
  endwin();
}
