//
// First own try...
// Using DHT 11 and a display to make a simple thermometer/hygrometer
//
// Todo:
// Fat16 library seems to mess up the card when opening/writing.
// other libraries are too big for me.
// Need to find out why the problem comes.

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

//
// Initiate display
//
// Pins are: MOSI, SCLK, CS, RESET, RS/DC
UTFT myGLCD(QD_TFT180C,4,5,9,8,7);
// Fonts
extern uint8_t SmallFont[];
//extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

// DHT11 setup

dht11 DHT11;

#define DHT11PIN 6

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
    Serial.printlfileName);
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
