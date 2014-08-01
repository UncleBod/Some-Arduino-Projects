//
// First own try...
// Using DHT 11 and a display to make a simple thermometer/hygrometer
//

//Debug
#define DEBUG

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
boolean SDCard; //Used to indicate if we have a good SDCard installed for the moment
char fileName[] = "APPEND00.TXT";

// Debugging variables
unsigned long maxFile=0;


// Variables for different display modes
int curDisplayMode=0, newDisplayMode=0;

// Variables for logging
unsigned long NextLogging;
// Exprimental - Time in ms between loggings (15 seconds now)
//MillisToNextLog = 1000UL*15UL;
const unsigned long PROGMEM MillisToNextLog = 1000UL*5UL;
int fcount;


void setup()
{
  Serial.begin(9600);
  byte EEPromInfo[5];
  int ypos=0;
  
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
  // 1 below is quarter speed. With only resistors to get the level conversion, better is not possible
  SDCard = card.init(1);
  if (SDCard)
  {
    myGLCD.print("SDCard found",CENTER,ypos);
    Serial.print("\nInitializing SD card...");
    // Here we will create the correct file and set the filename
    // The basicidea is: YYMMDDXX.CVS
    // This will give us 100 files per day, which should be enough, even with some crashes
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
  }
  ypos += 12;

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
  // check if it is time to write to SD card
  if (SDCard)
  {
    if (millis() > NextLogging)
    {
      writeFile();
      NextLogging += MillisToNextLog;
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
  
  myGLCD.setFont(BigFont);
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

  myGLCD.setFont(BigFont);
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
  myGLCD.print("NAD Weather Station",CENTER,0);
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
      returnvalue=file.sync();
      delay(50);
      returnvalue=file.close();
      delay(50);
      returnvalue=file.open(fileName, O_APPEND | O_WRITE);
      delay(50);
      returnvalue=file.seekEnd();
      delay(50);
      if (file.curPosition() < maxFile)
        SDCard = getNextFileName();
    #ifdef DEBUG
      Serial.print(file.curPosition(),DEC);
      Serial.print("  x  ");
      Serial.print(file.fileSize(),DEC);
      Serial.print("  x  ");
      Serial.println(maxFile,DEC);
    #endif
    }

    writeValues();
    // Check if everything is OK and close the file
    returnvalue = !(file.writeError);
    returnvalue &= file.sync();
    returnvalue &= file.close();
    // If somethign failed, we get the next filename
    if (!returnvalue)
      SDCard = getNextFileName();
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
  for (i = 0; i < 90; i++) {
    fileName[6] = i/10 + '0';
    fileName[7] = i%10 + '0';
    // If we can't open the file read only, we try to create it. If this succeeds, we have our new filename
    // O_CREAT - create the file if it does not exist
    // O_EXCL - fail if the file exists
    // O_WRITE - open for write
    // if (file.open(fileName, O_CREAT | O_EXCL | O_WRITE)) break;
    if (!file.open(fileName, O_READ))
    {
      file.close();
      if (file.open(fileName, O_CREAT | O_EXCL | O_WRITE)) break;
    }
    delay(50);
  }
  file.close();
  if (i < 88)
  {
    return true;
  }
  else
    return false;
}
