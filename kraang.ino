#include <SD.h>
#include "Buffer.hpp"
#include "Sequence.hpp"
#include "MIDIFile.hpp"
#include "Player.hpp"

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
    digitalWrite(13, HIGH);
    Serial1.write(event.type | event.channel);
    Serial1.write(event.param1);
    if ( event.type != Event::ProgChange )
      Serial1.write(event.param2);
    digitalWrite(13, LOW);
  }
};

class ArduinoTiming : public Timing
{
public:
  uint32_t getMicroseconds() const
  {
    return micros();
  }

  void delay(uint32_t us)
  {
    delayMicroseconds(us);
  }
};

class ArduinoFile : public KFile
{
private:
  File f;
public:
  ArduinoFile(const char *path)
  {
    f = SD.open(path, FILE_READ);
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

// globals
ArduinoMIDIPort midi_port;
Sequence sequence;
ArduinoTiming timing;
Player player{sequence, timing, midi_port};

void setup()
{
  while (!Serial);
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  SD.begin(SDCARD_SS_PIN);
  printDirectory("/");
  
  midi_port.init();

  Serial.println("Opening MIDI file");
  ArduinoFile file{"ANALOQ~2.MID"};
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
}

void loop()
{
  player.tick();
  /*Serial.print(player.getBpm());
  Serial.print('\t');
  Serial.print(player.getMeasure());
  Serial.print('\t');
  Serial.println(player.getBeat());*/
}

void printDirectory(char *path)
{
  File root = SD.open("/");
  while (true)
  {
    File entry = root.openNextFile();
    if (!entry)
      return;
    if (!entry.isDirectory())
    {
      // files have sizes, directories do not
      char *name = entry.name();
      if ( name[0] != '_' )
      {
        Serial.print(name);
        Serial.print("\t");
        Serial.println(entry.size(), DEC);
      }
    }
    entry.close();
  }
  root.close();
}
