//
// First own try...
// Using DHT 11 and a display to make a simple thermometer/hygrometer
//

#include <UTFTold.h>
#include <dht11.h>

//
// Initiate display
//

UTFT myGLCD(QD_TFT180C,11,10,9,8,7);
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
#define EEPromVersion 1
byte EEPromMagicInfo[] = {(byte)'U',(byte)'B',(byte)'W',(byte)'S',(byte)EEPromVersion};

// Setup for min/max temperature and humidity
byte minDHTTemp = 100;
byte maxDHTTemp = 0;
byte minDHTHum = 100;
byte maxDHTHum = 0;

byte EEminDHTTemp =100;
byte EEmaxDHTTemp = 0;
byte EEminDHTHum = 100;
byte EEmaxDHTHum = 0;

// Variables needed to read and show temperature values
float DHTcurrentTemp;
int DHTcurrentHumid;

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
  
// All setups ready
  myGLCD.fillScr(224, 224, 224);
  myGLCD.print("NAD Weather Station",CENTER,0);
}


void loop()
{
  DHTUpdate();
  DisplayInfo();
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
  }
}

//
// A first display routine
//

void DisplayInfo()
{
  int decimal;
  
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
}
