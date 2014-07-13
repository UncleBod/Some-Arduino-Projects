//
// First own try...
// Using DHT 11 and a display to make a simple thermometer/hygrometer
//

#include <UTFTold.h>
#include <dht11.h>
#include <EEPROM.h>
// Using Fat16 to minimize the memory used
#include <Fat16.h>
//#include <Fat16util.h> // use functions to print strings from flash memory

//
// Initiate display
//
// Pins are: MOSI, SCLK, CS, RESET, RS/DC
UTFT myGLCD(QD_TFT180C,4,5,9,8,7);
// Fonts
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

// DHT11 setup

dht11 DHT11;

#define DHT11PIN 6

// For gliding mean
#define DMaxBuf 8
#define DMaxBuf1 7
int myloop=0;
byte tempbuf[DMaxBuf];
boolean buffull=false;

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


// Variables needed to read and show temperature values
float DHTcurrentTemp;
int DHTcurrentHumid;

// Variables used for SDCard
SdCard card;
Fat16 file;
boolean SDCard;


// Variables for different display modes
int curDisplayMode=0, newDisplayMode=0;

void setup()
{
  byte EEPromInfo[5];
  int ypos=0;
  
// Setup the LCD
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(224, 224, 224);
  myGLCD.fillScr(224, 224, 224);
  myGLCD.setColor(255, 0, 0);
  myGLCD.print("Initiating system",CENTER,ypos);
  ypos +=12;
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
  if (!card.init(1))
  {
    myGLCD.print("SDCard Doesnt work",CENTER,ypos);
    ypos += 12;
    SDCard = false;
  }
  else
  {
    myGLCD.print("SDCard initiated",CENTER,ypos);
    ypos += 12;
    SDCard = true;
  }
// Read data from EEPRom
  myGLCD.print("Reading from EEProm",CENTER,ypos);
  ypos += 12;
  EEminDHTTemp = EEPROM.read(EEminDHTTempAdress);
  EEmaxDHTTemp = EEPROM.read(EEmaxDHTTempAdress);
  EEminDHTHumid = EEPROM.read(EEminDHTHumidAdress);
  EEmaxDHTHumid = EEPROM.read(EEmaxDHTHumidAdress);
  
  delay(5000);
// All setups ready
  myClearScreen();
}


void loop()
{
  DHTUpdate();
  DisplayInfo();
  newDisplayMode = (curDisplayMode + 1) & 3;
  delay(2000);
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
  }
  
  
  
  
  /*  int decimal;
  
  // Humidity from DHT
  myGLCD.setFont(SevenSegNumFont);
  myGLCD.printNumI((float)DHTcurrentHumid,RIGHT,24);
  
  // Temperature from DHT
  if (buffull)
  {
    myGLCD.printNumI((float)DHTcurrentTemp,80,74);
    decimal=(10*DHTcurrentTemp-10*int(DHTcurrentTemp));
    myGLCD.setFont(BigFont);
    myGLCD.printNumI((float)decimal,RIGHT,74);
  }
  else
  {
    myGLCD.printNumI((float)DHTcurrentTemp,80,74);
  }
  myGLCD.setFont(SmallFont);
  myGLCD.printNumI((float)minDHTHumid,LEFT,24,4);
  myGLCD.printNumI((float)maxDHTHumid,LEFT,36,4);yp
  myGLCD.printNumI((float)EEminDHTHumid,LEFT,48,4);
  myGLCD.printNumI((float)EEmaxDHTHumid,LEFT,60,4);

  myGLCD.printNumI((float)minDHTTemp,LEFT,72,4);
  myGLCD.printNumI((float)maxDHTTemp,LEFT,84,4);
  myGLCD.printNumI((float)EEminDHTTemp,LEFT,96,4);
  myGLCD.printNumI((float)EEmaxDHTTemp,LEFT,108,4);
*/  
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
    myGLCD.setFont(BigFont);
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

  myGLCD.print("  Min Values  ",CENTER,12);
}

// Display min values
void DisplayMax()
{

  myGLCD.print("  Max Values  ",CENTER,12);
}

// Display Graph
void DisplayGraph()
{

  myGLCD.print("Temp/Hum Graph",CENTER,12);
}

int initEEProm(int y)
{
  myGLCD.print("Initiating EEProm",CENTER,y);
  y += 12;
/*  for(int i=0; i<4; i++)
  {
    EEPROM.write(i,EEPromMagicInfo[i]);
  }
 
  // Write DHT max/min information
  EEPROM.write(EEminDHTTempAdress,EEminDHTTemp);
  EEPROM.write(EEmaxDHTTempAdress,EEmaxDHTTemp);
  EEPROM.write(EEminDHTHumidAdress,EEminDHTHumid);
  EEPROM.write(EEmaxDHTHumidAdress,EEmaxDHTHumid);*/
  return y;
}

// A simple clear screen that prints program name at top
void myClearScreen()
{
  myGLCD.setFont(SmallFont);
  myGLCD.fillScr(224, 224, 224);
  myGLCD.print("NAD Weather Station",CENTER,0);
}

