#include "SdFat.h"
#include "Adafruit_ZeroTimer.h"
#include "Buffer.hpp"
#include "Sequence.hpp"
#include "MIDIFile.hpp"
#include "Player.hpp"

// system globals
SdFat sd;
Adafruit_ZeroTimer zerotimer {Adafruit_ZeroTimer(3)};

class ArduinoMIDIPort : public MIDIPort
{
public:
  ArduinoMIDIPort()
  {
  }

  void init()
  {
    Serial1.begin(31250);
  }

  void send(const Event &event)
  {
    Serial1.write(event.type | event.channel);
    Serial1.write(event.param1);
    if ( event.type != Event::ProgChange )
      Serial1.write(event.param2);
  }
};

class ArduinoFile : public KFile
{
private:
  File f;
public:
  ArduinoFile(const char *path)
  {
    f = sd.open(path, O_RDONLY);
  }

  bool isValid()
  {
    return f ? true : false;
  }

  void close()
  {
    f.close();
  }

  uint8_t readByte()
  {
    return f.read();
  }

  void read(uint32_t length, uint8_t *data)
  {
    f.read(data, length);
  }

  uint32_t getPosition()
  {
    return f.position();
  }

  void seek(int32_t position)
  {
    f.seek(f.position() + position);
  }
};

// application globals
ArduinoMIDIPort midi_port;
Sequence sequence;
Player player{sequence, midi_port};

void TC3_Handler()
{
  Adafruit_ZeroTimer::timerHandler(3);
}

void TimerCallback0(void)
{
  player.tick();
  zerotimer.setCompare(0, player.getDelay() * 48 / 4);
}

void setup()
{
  // initialize
  while (!Serial) delay(10);
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  sd.begin(SDCARD_SS_PIN, SD_SCK_MHZ(50));
  printDirectory("/");
  midi_port.init();

  // load sequence
  Serial.println("Opening MIDI file");
  ArduinoFile file{"analoq_fflegends.mid"};
  if ( file.isValid() )
    Serial.println("File Valid");
  else
    Serial.println("File Invalid");
  Serial.println("Loading reader");
  MIDIFile midi_file{file};
  Serial.println("Reading MIDI file");
  midi_file.import(sequence);
  Serial.print("Ticks: ");
  Serial.println(sequence.getTicks());
  Serial.print("Delay: ");
  Serial.println(player.getDelay());

  // setup timer
  uint16_t compare = player.getDelay() * 48 / 4;
  zerotimer.enable(false);
  zerotimer.configure(TC_CLOCK_PRESCALER_DIV4,       // prescaler
          TC_COUNTER_SIZE_16BIT,       // bit width of timer/counter
          TC_WAVE_GENERATION_MATCH_PWM // frequency or PWM mode
          );
  zerotimer.setCompare(0, compare);
  zerotimer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, TimerCallback0);
  zerotimer.enable(true);
}

void loop()
{
  Serial.print(player.getBpm());
  Serial.print('\t');
  Serial.print(player.getMeasure()+1);
  Serial.print('\t');
  Serial.println(player.getBeat()+1);
  delay(500);
}

void printDirectory(char *path)
{
  SdFile file;
  SdFile dir;

  dir.open(path, O_RDONLY);
  while ( file.openNext(&dir, O_RDONLY) )
  {
    if ( !file.isSubDir() && !file.isHidden() )
    {
      char name[16];
      file.getName(name, 16);
      Serial.print(name);
      Serial.print('\t');
      Serial.println(file.fileSize());
    }
    file.close();
  }
  dir.close();
}
