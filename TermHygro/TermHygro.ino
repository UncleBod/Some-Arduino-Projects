// 28482 of 30720 bytes...
// First own try...
// Using DHT 11 and a display to make a simple thermometer/hygrometer
//
// Todo:
// Fat16 library seems to mess up the card when opening/writing.
// other libraries are too big for me.
// Need to find out why the problem comes.
// 
// 2014-11-17
// Update pins for future.
// Old  New
// D9 - D5
// D8 - D8
// D7 - D7
// D6 - D9
// D5 - D3
// D4 - D4
// D3 - D4
// D2 - D3
// Note New D4 and D3 will have two connections each.
// D6 will be TFT Backligt (adjustable light)
// TOCHECK - will TFT and RTC work when both uses the same pin? Pulldown resistors?
// Binary sketch size: 25 966 bytes (of a 30 720 byte maximum)
//
// 2014-09-23
// Start changing for temporary removing SD card and using EErom for logging

//Debug
//#define DEBUG

#include <UTFTold.h>
#include <dht11.h>
#include <EEPROM.h>
// Using Fat16 to minimize the memory used
#include <Fat16.h>
//#include <Fat16util.h> // use functions to print strings from flash memory

// OneWire for DS18B20 temp sensor
#include <OneWire.h>

//
// Initiate display
//
// Pins are: MOSI, SCLK, CS, RESET, RS/DC
UTFT myGLCD(QD_TFT180C,4,3,5,8,7);
// UTFT myGLCD(QD_TFT180C,4,5,9,8,7);
// Fonts
extern uint8_t SmallFont[];
//extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

// DHT11 setup

dht11 DHT11;

#define DHT11PIN 9
//#define DHT11PIN 6

// DS18B20 init
// DS18S20 Temperature chip i/o
OneWire ds(1);// on pin 1

// For monitoring the Vin
int sensorValue;
float vin;

// For gliding mean
#define DMaxBuf 8
#define DMaxBuf1 7
int myloop=0;
byte tempbuf[DMaxBuf];
boolean buffull=false;

/*
EEProm memory map:
Adr Cont
00  'U'
01  'B'
02  'W'
03  'S'
04  Version
05  min DHT temp
06  max DHT temp
07  min DHT humidity
08  max DHT humidity
09  min probe 2
10  max probe 2
11  min probe 3
12  max probe 3
13  logg type and status (round robin or one time) (8 bits?)
  b0  RR (0 = one time, 1 = round robin)
  b1  Reboot activity (0 = restart logg, 1 = continue logg)
  b2  first round (0 = filling first time, 1 = filled at least once)
  b3
  b4
  b5
  b6
  b7
14,15 logg frequence
16  logg place
17  
18
19
20
21
22
23

24
--  Logged data - 4 variables, 250 points each
1023
*/
// Magic bits for the EEProm
#define EEPromVersion 2
byte EEPromMagicInfo[] = {(byte)'U',(byte)'B',(byte)'W',(byte)'S',(byte)EEPromVersion};

// Setup for min/max temperature and humidity
byte minDHTTemp = 100;
byte maxDHTTemp = 0;
byte minDHTHumid = 100;
byte maxDHTHumid = 0;

byte EEminDHTTemp =100;
byte EEmaxDHTTemp = 0;
byte EEminDHTHumid = 100;
byte EEmaxDHTHumid = 0;
#define EEminDHTTempAdress 5
#define EEmaxDHTTempAdress 6
#define EEminDHTHumidAdress 7
#define EEmaxDHTHumidAdress 8

// EEProm logging variables and constants
#define EELoggType        1    // RR (0 = one time, 1 = round robin)
#define EERebootActivity  2    // Reboot activity (0 = restart logg, 1 = continue logg)
#define EELoggStatus      4    // first round (0 = filling first time, 1 = filled at least once)

byte currLogg;


// Variables needed to read and show temperature values
float DHTcurrentTemp;
int DHTcurrentHumid;

// Variables used for SDCard
SdCard card;
Fat16 file;
boolean SDCard =false; //Used to indicate if we have a good SDCard installed for the moment
char fileName[] = "APPEND00.TXT";
byte lastNameIndex = 0;

// Debugging variables
unsigned long maxFile=0;


// Variables for different display modes
int curDisplayMode=0, newDisplayMode=0;

// Variables for logging
unsigned long NextLogging;
// Exprimental - Time in ms between loggings (15 seconds now)
const unsigned long PROGMEM MillisToNextLog = 1000UL*15UL;
int fcount;

// Defines for RTC
// TODO Most of this should be in a header file.
// Set your own pins with these defines !
// A wild cutnpaste from arduino.cc user "Krodal"
//
// TODO 4 is shared pin, have to remake!
//
#define DS1302_SCLK_PIN   3 // 2 // Arduino pin for the Serial Clock
#define DS1302_IO_PIN     4 // 3 // Arduino pin for the Data I/O
#define DS1302_CE_PIN     2 // 4 // Arduino pin for the Chip Enable


// Macros to convert the bcd values of the registers to normal
// integer variables.
// The code uses seperate variables for the high byte and the low byte
// of the bcd, so these macros handle both bytes seperately.
#define bcd2bin(h,l)    (((h)*10) + (l))
#define bin2bcd_h(x)   ((x)/10)
#define bin2bcd_l(x)    ((x)%10)


// Register names.
// Since the highest bit is always '1', 
// the registers start at 0x80
// If the register is read, the lowest bit should be '1'.
#define DS1302_SECONDS           0x80
#define DS1302_MINUTES           0x82
#define DS1302_HOURS             0x84
#define DS1302_DATE              0x86
#define DS1302_MONTH             0x88
#define DS1302_DAY               0x8A
#define DS1302_YEAR              0x8C
#define DS1302_ENABLE            0x8E
#define DS1302_TRICKLE           0x90
#define DS1302_CLOCK_BURST       0xBE
#define DS1302_CLOCK_BURST_WRITE 0xBE
#define DS1302_CLOCK_BURST_READ  0xBF
#define DS1302_RAMSTART          0xC0
#define DS1302_RAMEND            0xFC
#define DS1302_RAM_BURST         0xFE
#define DS1302_RAM_BURST_WRITE   0xFE
#define DS1302_RAM_BURST_READ    0xFF



// Defines for the bits, to be able to change 
// between bit number and binary definition.
// By using the bit number, using the DS1302 
// is like programming an AVR microcontroller.
// But instead of using "(1<<X)", or "_BV(X)", 
// the Arduino "bit(X)" is used.
#define DS1302_D0 0
#define DS1302_D1 1
#define DS1302_D2 2
#define DS1302_D3 3
#define DS1302_D4 4
#define DS1302_D5 5
#define DS1302_D6 6
#define DS1302_D7 7


// Bit for reading (bit in address)
#define DS1302_READBIT DS1302_D0 // READBIT=1: read instruction

// Bit for clock (0) or ram (1) area, 
// called R/C-bit (bit in address)
#define DS1302_RC DS1302_D6

// Seconds Register
#define DS1302_CH DS1302_D7   // 1 = Clock Halt, 0 = start

// Hour Register
#define DS1302_AM_PM DS1302_D5 // 0 = AM, 1 = PM
#define DS1302_12_24 DS1302_D7 // 0 = 24 hour, 1 = 12 hour

// Enable Register
#define DS1302_WP DS1302_D7   // 1 = Write Protect, 0 = enabled

// Trickle Register
#define DS1302_ROUT0 DS1302_D0
#define DS1302_ROUT1 DS1302_D1
#define DS1302_DS0   DS1302_D2
#define DS1302_DS1   DS1302_D2
#define DS1302_TCS0  DS1302_D4
#define DS1302_TCS1  DS1302_D5
#define DS1302_TCS2  DS1302_D6
#define DS1302_TCS3  DS1302_D7


// Structure for the first 8 registers.
// These 8 bytes can be read at once with 
// the 'clock burst' command.
// Note that this structure contains an anonymous union.
// It might cause a problem on other compilers.
typedef struct ds1302_struct
{
  uint8_t Seconds:4;      // low decimal digit 0-9
  uint8_t Seconds10:3;    // high decimal digit 0-5
  uint8_t CH:1;           // CH = Clock Halt
  uint8_t Minutes:4;
  uint8_t Minutes10:3;
  uint8_t reserved1:1;
  union
  {
    struct
    {
      uint8_t Hour:4;
      uint8_t Hour10:2;
      uint8_t reserved2:1;
      uint8_t hour_12_24:1; // 0 for 24 hour format
    } h24;
    struct
    {
      uint8_t Hour:4;
      uint8_t Hour10:1;
      uint8_t AM_PM:1;      // 0 for AM, 1 for PM
      uint8_t reserved2:1;
      uint8_t hour_12_24:1; // 1 for 12 hour format
    } h12;
  };
  uint8_t Date:4;           // Day of month, 1 = first day
  uint8_t Date10:2;
  uint8_t reserved3:2;
  uint8_t Month:4;          // Month, 1 = January
  uint8_t Month10:1;
  uint8_t reserved4:3;
  uint8_t Day:3;            // Day of week, 1 = first day (any day)
  uint8_t reserved5:5;
  uint8_t Year:4;           // Year, 0 = year 2000
  uint8_t Year10:4;
  uint8_t reserved6:7;
  uint8_t WP:1;             // WP = Write Protect
};



void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
#endif
  byte EEPromInfo[5];
  int ypos=0;
  analogReference(INTERNAL);
// Setup the LCD
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(224, 224, 224);
  myGLCD.fillScr(224, 224, 224);
  myGLCD.setColor(5, 5, 5);
  myGLCD.print("Initiating system",CENTER,ypos);
  ypos +=12;
  myGLCD.print("Starting RTC",CENTER,ypos);
  ypos +=12;
// Setup for RTC
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  DS1302_write (DS1302_ENABLE, 0);

  // Disable Trickle Charger.
  DS1302_write (DS1302_TRICKLE, 0x00);
// Check if EEProm is initiated.
  myGLCD.print("Checking EEProm",CENTER,ypos);
  ypos += 12;
  for(int i=0; i<4; i++)
  {
    if ((byte)EEPROM.read(i) != EEPromMagicInfo[i])
    {
      ypos = initEEProm(ypos);
      break;
    }
  }
  myGLCD.print("Checking SDCard",CENTER,ypos);
  ypos += 12;
  SDCard = false;
  if (SDCard)
  {
    // 1 below is quarter speed. With only resistors to get the level conversion, better is not possible
    SDCard = card.init(1);
    if (SDCard)
    {
      myGLCD.print("SDCard found",CENTER,ypos);
#ifdef DEBUG
      Serial.print("\nInitializing SD card...");
#endif
      // Here we will create the correct file and set the filename
      // The basic idea is: YYMMDDXX.CVS (XX = 01-99)
      // This will give us 99 files per day, which should be enough, even with some crashes
      if (Fat16::init(&card))
        SDCard = getNextFileName();
      else
        SDCard=false;
    }
    ypos += 12;
    if (!SDCard)
    {
      myGLCD.setColor(255, 0, 0);
      myGLCD.print("SDCard Doesnt work",CENTER,ypos);
      myGLCD.setColor(5, 5, 5);

    }
    else
    {
      myGLCD.setColor(0, 255, 0);
      myGLCD.print("SDCard initiated",CENTER,ypos);
      myGLCD.setColor(5, 5, 5);
      ypos += 12;
      myGLCD.print(fileName,CENTER,ypos);
    }
    ypos += 12;
  }
// Read data from EEPRom
  myGLCD.print("Reading from EEProm",CENTER,ypos);
  ypos += 12;
  EEminDHTTemp = EEPROM.read(EEminDHTTempAdress);
  EEmaxDHTTemp = EEPROM.read(EEmaxDHTTempAdress);
  EEminDHTHumid = EEPROM.read(EEminDHTHumidAdress);
  EEmaxDHTHumid = EEPROM.read(EEmaxDHTHumidAdress);
  

  delay(1000);
// All setups ready
  myClearScreen();
  NextLogging=millis() + MillisToNextLog;
}


void loop()
{
  DHTUpdate();
  // A7 is 1/11th of the Vin
  sensorValue = analogRead(A7);            
  vin=1.1/1024*sensorValue*11.0;

  // check if it is time to write to SD card
  if (SDCard)
  {
    if (millis() > NextLogging)
    {
      if (SDCard) writeFile();
      NextLogging += (unsigned long)MillisToNextLog;
    }
  }
  DisplayInfo();
  newDisplayMode = curDisplayMode + 1;
  if (newDisplayMode > 4) newDisplayMode = 0;
  delay(2000);



// For testing size needed for DS18B20
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      Serial.print("No more addresses.\n");
      ds.reset_search();
      return;
  }

  Serial.print("R=");
  for( i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
  }
/*
  if ( addr[0] == 0x10) {
      Serial.print("Device is a DS18S20 family device.\n");
  }
  else if ( addr[0] == 0x28) {
      Serial.print("Device is a DS18B20 family device.\n");
  }
  else {
      Serial.print("Device family is not recognized: 0x");
      Serial.println(addr[0],HEX);
      return;
  }
*/
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("P=");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print( OneWire::crc8( data, 8), HEX);
  Serial.println();


}

//
// Read temp adn humidity, set max and min values if changed
// 
void DHTUpdate()
{
  int currentTemp;
  int temp=0;
  float meanTemp;
  int chk = DHT11.read(DHT11PIN);
  if (chk==DHTLIB_OK)
  {
    currentTemp=(byte)DHT11.temperature;
    DHTcurrentHumid=(byte)DHT11.humidity;
    tempbuf[myloop]=currentTemp;
    myloop=(++myloop&DMaxBuf1);
    buffull|=(myloop==0);
    for(int i=0;i<DMaxBuf;i++)
    {
       temp+=(int)tempbuf[i];
    }
    meanTemp=(float)temp/(float)DMaxBuf;
    if (buffull)
    {
      DHTcurrentTemp=meanTemp;
    }
    else
    {
      DHTcurrentTemp=(float)currentTemp;
    }
    
    //Setup max/min values
    if (DHTcurrentTemp < minDHTTemp)
    {
      minDHTTemp = (byte)DHTcurrentTemp;
    }
    if (DHTcurrentTemp < EEminDHTTemp)
    {
      EEminDHTTemp = (byte)DHTcurrentTemp;
      EEPROM.write(EEminDHTTempAdress,EEminDHTTemp);
    }
    if (DHTcurrentTemp > maxDHTTemp)
    {
      maxDHTTemp = (byte)DHTcurrentTemp;
    }
    if (DHTcurrentTemp > EEmaxDHTTemp)
    {
      EEmaxDHTTemp = (byte)DHTcurrentTemp;
      EEPROM.write(EEmaxDHTTempAdress,EEmaxDHTTemp);
    }
    if (DHTcurrentHumid < minDHTHumid)
    {
      minDHTHumid = DHTcurrentHumid;
    }
    if (DHTcurrentHumid < EEminDHTHumid)
    {
      EEminDHTHumid = (byte)DHTcurrentHumid;
      EEPROM.write(EEminDHTHumidAdress,EEminDHTHumid);
    }
    if (DHTcurrentHumid > maxDHTHumid)
    {
      maxDHTHumid = (byte)DHTcurrentHumid;
    }
    if (DHTcurrentHumid > EEmaxDHTHumid)
    {
      EEmaxDHTHumid = (byte)DHTcurrentHumid;
      EEPROM.write(EEmaxDHTHumidAdress,EEmaxDHTHumid);
    }
  }
}

//
// A first display routine
//

void DisplayInfo()
{
  if (curDisplayMode != newDisplayMode)
  {
    myClearScreen();
    curDisplayMode = newDisplayMode;
  }
  switch(curDisplayMode)
  {
    case 0:
      DispalyCurrent();
      break;
    case 1:
      DisplayMin();
      break;
    case 2:
      DisplayMax();
      break;
    case 3:
      DisplayGraph();
      break;
    case 4:
      DisplayClock();
      break;
  }
}

// Display current values
void DispalyCurrent()
{
  int decimal;
  
  myGLCD.print("Current Values",CENTER,12);
  // Humidity from DHT
  myGLCD.setFont(SevenSegNumFont);
  myGLCD.printNumI((float)DHTcurrentHumid,RIGHT,24);
  
  // Temperature from DHT
  if (buffull)
  {
    myGLCD.printNumI((float)DHTcurrentTemp,80,74);
    decimal=(10*DHTcurrentTemp-10*int(DHTcurrentTemp));
    //myGLCD.setFont(BigFont);
    myGLCD.setFont(SmallFont);
    myGLCD.printNumI((float)decimal,RIGHT,74);
  }
  else
  {
    myGLCD.printNumI((float)DHTcurrentTemp,80,74);
  }
}

// Display min values
void DisplayMin()
{

  myGLCD.print("EE   Min Values  Cur",CENTER,12);
  myGLCD.setFont(SevenSegNumFont);
  
  // Min humidity from DHT
  myGLCD.printNumI((float)minDHTHumid,RIGHT,24);
  // Min temperature from DHT
  myGLCD.printNumI((float)minDHTTemp,RIGHT,74);
  
  // Min humidity from EE
  myGLCD.printNumI((float)EEminDHTHumid,LEFT,24);
  // Min temperature from EE
  myGLCD.printNumI((float)EEminDHTTemp,LEFT,74);
  
  myGLCD.setFont(SmallFont);
}

// Display min values
void DisplayMax()
{

  myGLCD.print("EE   Max Values  Cur",CENTER,12);
  myGLCD.setFont(SevenSegNumFont);
  
  // Max humidity from DHT
  myGLCD.printNumI((float)maxDHTHumid,RIGHT,24);
  // Max temperature from DHT
  myGLCD.printNumI((float)maxDHTTemp,RIGHT,74);
  
  // Max humidity from EE
  myGLCD.printNumI((float)EEmaxDHTHumid,LEFT,24);
  // Max temperature from EE
  myGLCD.printNumI((float)EEmaxDHTTemp,LEFT,74);

  myGLCD.setFont(SmallFont);
}

// Display Graph
void DisplayGraph()
{

  myGLCD.print("Temp/Hum Graph",CENTER,12);
  int buf[158];
  int x;
  int y;
  myGLCD.setColor(0, 0, 25);
  myGLCD.setBackColor(224, 224, 224);
  myGLCD.drawLine(79, 14, 79, 113);
  myGLCD.drawLine(1, 63, 158, 63);

  x=1;
  for (int i=1; i<(158*20); i++) 
  {
    x++;
    if (x==159)
      x=1;
    if (i>159)
    {
      if ((x==79)||(buf[x-1]==63))
        myGLCD.setColor(0,0,25);
      else
        myGLCD.setColor(224,224,224);
      myGLCD.drawPixel(x,buf[x-1]);
    }
    myGLCD.setColor(0,25,25);
    y=63+(sin(((i*2.5)*3.14)/180)*(40-(i / 100)));
    myGLCD.drawPixel(x,y);
    buf[x-1]=y;
  }

}

//Dipsplay the clock and date

void DisplayClock()
{
  ds1302_struct rtc;
  char buffer[80];     // the code uses 70 characters.

  // Read all clock data at once (burst mode).
  DS1302_clock_burst_read( (uint8_t *) &rtc);

  
  myGLCD.print("Time and date",CENTER,12);
  myGLCD.setFont(SevenSegNumFont);
  
  // Hour min second
  myGLCD.printNumI((float)bcd2bin( rtc.h24.Hour10, rtc.h24.Hour),LEFT,24);
  myGLCD.printNumI((float)bcd2bin( rtc.Minutes10, rtc.Minutes),RIGHT,24);
  //myGLCD.printNumI((float)bcd2bin( rtc.Seconds10, rtc.Seconds),RIGHT,24);

  // Year Month Day
  // myGLCD.printNumI((float)bcd2bin( rtc.Year10, rtc.Year),LEFT,74);
  myGLCD.printNumI((float)bcd2bin( rtc.Month10, rtc.Month),LEFT,74);
  myGLCD.printNumI((float)bcd2bin( rtc.Date10, rtc.Date),RIGHT,74);

  myGLCD.setFont(SmallFont);
}

int initEEProm(int y)
{
  myGLCD.print("Initiating EEProm",CENTER,y);
  y += 12;
  for(int i=0; i<4; i++)
  {
    EEPROM.write(i,EEPromMagicInfo[i]);
  }
 
  // Write DHT max/min information
  EEPROM.write(EEminDHTTempAdress,EEminDHTTemp);
  EEPROM.write(EEmaxDHTTempAdress,EEmaxDHTTemp);
  EEPROM.write(EEminDHTHumidAdress,EEminDHTHumid);
  EEPROM.write(EEmaxDHTHumidAdress,EEmaxDHTHumid);
  return y;
}

// A simple clear screen that prints program name at top
void myClearScreen()
{
  myGLCD.setFont(SmallFont);
  myGLCD.fillScr(224, 224, 224);
  //myGLCD.print("NAD Weather Station",CENTER,0);
  myGLCD.printNumF((float)vin,2,CENTER,0);
}




void writeFile()
{
  boolean returnvalue;
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(255, 5, 5);
  myGLCD.print(fileName,CENTER,12);
  myGLCD.setColor(5, 5, 5);
  
  
  // O_APPEND - seek to the end of the file prior to each write
  // O_WRITE - open for write
  // if (!file.open(name, O_CREAT | O_APPEND | O_WRITE)) Serial.println("open error");
  if (file.open(fileName, O_APPEND | O_WRITE))
  {
    returnvalue=file.seekEnd();
    #ifdef DEBUG
    Serial.print("\nWriting to SD card...");
    Serial.println(fileName);
    Serial.print(file.curPosition(),DEC);
    Serial.print("  -  ");
    Serial.print(file.fileSize(),DEC);
    Serial.print("  -  ");
    Serial.println(maxFile,DEC);
    #endif
    // Are we at end of file? If not, do something!
    // FAT16write has a simple idea about naming files with numbers
    // If something goes wrong during write etc the best is probably to close the file and create a new. 
    // In that way it should be possible to minimise data loss.
    if (file.curPosition() < maxFile)
    {
      myGLCD.print("Sync Error!!!!!",CENTER,12);
      returnvalue=file.sync();
      delay(50);
      returnvalue=file.close();
      delay(50);
      SDCard = getNextFileName();
      returnvalue=file.open(fileName, O_APPEND | O_WRITE);
      delay(50);
      returnvalue=file.seekEnd();
      delay(50);
      //if (file.curPosition() < maxFile)
      //  SDCard = getNextFileName();
    #ifdef DEBUG
      Serial.print(file.curPosition(),DEC);
      Serial.print("  x  ");
      Serial.print(file.fileSize(),DEC);
      Serial.print("  x  ");
      Serial.println(maxFile,DEC);
    #endif
    }

    writeValues();
    if (file.writeError)
    {
      returnvalue = true;
    #ifdef DEBUG
      Serial.print("writeError");
    #endif
    }
    else
      returnvalue = false;

    // Check if everything is OK and close the file
    // returnvalue = !(file.writeError);
    // returnvalue &= file.sync();
    // returnvalue &= file.close();
    // If something failed, we get the next filename
    if (!file.close() || returnvalue)
      SDCard = getNextFileName();
  }
  // File failed to open.
  // Get a new filename for the next time
  // Todo:
  // Try to open and write to the new file.
  else
  {
    SDCard = getNextFileName();
    if (SDCard)
    {
     file.open(fileName, O_APPEND | O_WRITE);
     writeValues();
    }
  }
  returnvalue=file.close();
}

void writeValues()
{
    // print current data
    file.print(fcount++, DEC);
    file.print(",");
    file.print(DHTcurrentHumid, DEC);
    file.print(",");
    file.print((int)DHTcurrentTemp, DEC);
    file.print(",");
    file.print(file.curPosition(),DEC);
    file.print(",");
    file.print(file.fileSize(),DEC);
    file.print(",");
    if (file.fileSize() > maxFile) maxFile=file.fileSize();
    file.print(maxFile,DEC);
    file.print(",");
    file.println(millis());
    file.sync();
    // Wait 50 ms to give the ststem time to write...
    delay(50);
    file.sync();
}

boolean getNextFileName()
{
  int i;
  // Try to open a file until the filename is unused
  // TODO
  // If the counter ends, the function should return FALSE
  for (i = lastNameIndex + 1; i < 100; i++) {
    fileName[6] = i/10 + '0';
    fileName[7] = i%10 + '0';
    // If we can't open the file read only, we try to create it. If this succeeds, we have our new filename
    // O_CREAT - create the file if it does not exist
    // O_EXCL - fail if the file exists
    // O_WRITE - open for write
    // if (file.open(fileName, O_CREAT | O_EXCL | O_WRITE)) break;
    #ifdef DEBUG
      Serial.print("Trying ");
      Serial.println(fileName);
      Serial.print("Open Read ");
    #endif
    if (!file.open(fileName, O_READ))
    {
    #ifdef DEBUG
      Serial.print("Failed");
    #endif
      file.close();
      if (file.open(fileName, O_CREAT | O_EXCL | O_WRITE)) break;
    #ifdef DEBUG
      Serial.print("Open create Failed ");
    #endif
    }
    delay(50);
  }
  file.close();
  lastNameIndex = i;
  maxFile=0;
  if (i < 99)
    return true;
  else
    return false;
}

// EEProm Log
// Write value to the next place in the logg
// Keep track of position and what to do if the position overflows
//
// IN
//
// returns
// 0 OK
// 1 End of logging
// 2 Write error



// Routines for RTC
// --------------------------------------------------------
// DS1302_clock_burst_read
//
// This function reads 8 bytes clock data in burst mode
// from the DS1302.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void DS1302_clock_burst_read( uint8_t *p)
{
  int i;

  _DS1302_start();

  // Instead of the address, 
  // the CLOCK_BURST_READ command is issued
  // the I/O-line is released for the data
  _DS1302_togglewrite( DS1302_CLOCK_BURST_READ, true);  

  for( i=0; i<8; i++)
  {
    *p++ = _DS1302_toggleread();
  }
  _DS1302_stop();
}


// --------------------------------------------------------
// DS1302_clock_burst_write
//
// This function writes 8 bytes clock data in burst mode
// to the DS1302.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void DS1302_clock_burst_write( uint8_t *p)
{
  int i;

  _DS1302_start();

  // Instead of the address, 
  // the CLOCK_BURST_WRITE command is issued.
  // the I/O-line is not released
  _DS1302_togglewrite( DS1302_CLOCK_BURST_WRITE, false);  

  for( i=0; i<8; i++)
  {
    // the I/O-line is not released
    _DS1302_togglewrite( *p++, false);  
  }
  _DS1302_stop();
}


// --------------------------------------------------------
// DS1302_read
//
// This function reads a byte from the DS1302 
// (clock or ram).
//
// The address could be like "0x80" or "0x81", 
// the lowest bit is set anyway.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
uint8_t DS1302_read(int address)
{
  uint8_t data;

  // set lowest bit (read bit) in address
  bitSet( address, DS1302_READBIT);  

  _DS1302_start();
  // the I/O-line is released for the data
  _DS1302_togglewrite( address, true);  
  data = _DS1302_toggleread();
  _DS1302_stop();

  return (data);
}


// --------------------------------------------------------
// DS1302_write
//
// This function writes a byte to the DS1302 (clock or ram).
//
// The address could be like "0x80" or "0x81", 
// the lowest bit is cleared anyway.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void DS1302_write( int address, uint8_t data)
{
  // clear lowest bit (read bit) in address
  bitClear( address, DS1302_READBIT);   

  _DS1302_start();
  // don't release the I/O-line
  _DS1302_togglewrite( address, false); 
  // don't release the I/O-line
  _DS1302_togglewrite( data, false); 
  _DS1302_stop();  
}


// --------------------------------------------------------
// _DS1302_start
//
// A helper function to setup the start condition.
//
// An 'init' function is not used.
// But now the pinMode is set every time.
// That's not a big deal, and it's valid.
// At startup, the pins of the Arduino are high impedance.
// Since the DS1302 has pull-down resistors, 
// the signals are low (inactive) until the DS1302 is used.
void _DS1302_start( void)
{
  digitalWrite( DS1302_CE_PIN, LOW); // default, not enabled
  pinMode( DS1302_CE_PIN, OUTPUT);  

  digitalWrite( DS1302_SCLK_PIN, LOW); // default, clock low
  pinMode( DS1302_SCLK_PIN, OUTPUT);

  pinMode( DS1302_IO_PIN, OUTPUT);

  digitalWrite( DS1302_CE_PIN, HIGH); // start the session
  delayMicroseconds( 4);           // tCC = 4us
}


// --------------------------------------------------------
// _DS1302_stop
//
// A helper function to finish the communication.
//
void _DS1302_stop(void)
{
  // Set CE low
  digitalWrite( DS1302_CE_PIN, LOW);

  delayMicroseconds( 4);           // tCWH = 4us
}


// --------------------------------------------------------
// _DS1302_toggleread
//
// A helper function for reading a byte with bit toggle
//
// This function assumes that the SCLK is still high.
//
uint8_t _DS1302_toggleread( void)
{
  uint8_t i, data;

  data = 0;
  for( i = 0; i <= 7; i++)
  {
    // Issue a clock pulse for the next databit.
    // If the 'togglewrite' function was used before 
    // this function, the SCLK is already high.
    digitalWrite( DS1302_SCLK_PIN, HIGH);
    delayMicroseconds( 1);

    // Clock down, data is ready after some time.
    digitalWrite( DS1302_SCLK_PIN, LOW);
    delayMicroseconds( 1);        // tCL=1000ns, tCDD=800ns

    // read bit, and set it in place in 'data' variable
    bitWrite( data, i, digitalRead( DS1302_IO_PIN)); 
  }
  return( data);
}


// --------------------------------------------------------
// _DS1302_togglewrite
//
// A helper function for writing a byte with bit toggle
//
// The 'release' parameter is for a read after this write.
// It will release the I/O-line and will keep the SCLK high.
//
void _DS1302_togglewrite( uint8_t data, uint8_t release)
{
  int i;

  for( i = 0; i <= 7; i++)
  { 
    // set a bit of the data on the I/O-line
    digitalWrite( DS1302_IO_PIN, bitRead(data, i));  
    delayMicroseconds( 1);     // tDC = 200ns

    // clock up, data is read by DS1302
    digitalWrite( DS1302_SCLK_PIN, HIGH);     
    delayMicroseconds( 1);     // tCH = 1000ns, tCDH = 800ns

    if( release && i == 7)
    {
      // If this write is followed by a read, 
      // the I/O-line should be released after 
      // the last bit, before the clock line is made low.
      // This is according the datasheet.
      // I have seen other programs that don't release 
      // the I/O-line at this moment,
      // and that could cause a shortcut spike 
      // on the I/O-line.
      pinMode( DS1302_IO_PIN, INPUT);

      // For Arduino 1.0.3, removing the pull-up is no longer needed.
      // Setting the pin as 'INPUT' will already remove the pull-up.
      // digitalWrite (DS1302_IO, LOW); // remove any pull-up  
    }
    else
    {
      digitalWrite( DS1302_SCLK_PIN, LOW);
      delayMicroseconds( 1);       // tCL=1000ns, tCDD=800ns
    }
  }
}

//
// Key decoder routines
//
// Keyboard is connected to analog input with a voltage devider
//

