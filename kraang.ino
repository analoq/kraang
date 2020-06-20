#include "SdFat.h"
#include "Adafruit_ZeroTimer.h"
#include "Adafruit_RGBLCDShield.h"

#include "Buffer.hpp"
#include "Sequence.hpp"
#include "MIDIFile.hpp"
#include "Player.hpp"

// system globals
SdFat sd;
Adafruit_ZeroTimer zerotimer {Adafruit_ZeroTimer(3)};
Adafruit_RGBLCDShield lcd;

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
  // initialize serial comm
  while (!Serial) delay(10);
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  // initialize sd card
  sd.begin(SDCARD_SS_PIN, SD_SCK_MHZ(50));

  // initialize LCD
  lcd.begin(16, 2);
  lcd.setBacklight(0x7);

  // initialize midi
  midi_port.init();

  ui_choose_file();
}

void start_timer()
{
  player.returnToZero();
  
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

void stop_timer()
{
  zerotimer.enable(false);
}

bool nextFileName(SdFile &dir, char longname[16], char shortname[13])
{
  SdFile file;
  while ( file.openNext(&dir, O_RDONLY) )
  {
    if ( !file.isSubDir() && !file.isHidden() )
    {
      file.getName(longname, 16);
      file.getSFN(shortname);
      file.close();
      return true;
    }
    file.close();
  }
  return false;
}

void ui_choose_file()
{
  char shortname[13];
  SdFile dir;
  dir.open("/", O_RDONLY);
  while ( true )
  {
    char longname[16];
    if ( !nextFileName(dir, longname, shortname) )
    {
      dir.rewind();
      continue;
    }
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Select File:");
    lcd.setCursor(0,1);
    lcd.print(longname);
  
    uint8_t buttons;
    while ( !(buttons = lcd.readButtons()) );
    if ( buttons & BUTTON_SELECT )
      break;
    while ( lcd.readButtons() );
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    Loading...  ");
  
  // load sequence
  Serial.println("Opening MIDI file");
  ArduinoFile file{shortname};
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

  start_timer();
  
  lcd.clear();
}

void loop()
{
  lcd.setCursor(0,0);
  lcd.print(player.getBpm());
  lcd.print(" BPM");
  lcd.setCursor(0,1);
  lcd.print(player.getMeasure()+1);
  lcd.print(":");
  lcd.print(player.getBeat()+1);
  delay(200);

  uint8_t buttons = lcd.readButtons();
  if ( buttons )
  {
    stop_timer();
    ui_choose_file();
  }
/*  Serial.print(player.getBpm());
  Serial.print('\t');
  Serial.print(player.getMeasure()+1);
  Serial.print('\t');
  Serial.println(player.getBeat()+1);
  delay(500);*/
}

/*void printDirectory(char *path)
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
}*/
