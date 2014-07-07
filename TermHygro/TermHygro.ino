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

// Other inits
#define DMaxBuf 8
#define DMaxBuf1 7
int myloop=0;
byte tempbuf[DMaxBuf];
boolean buffull=false;

void setup()
{
  randomSeed(analogRead(0));
  
// Setup the LCD
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(224, 224, 224);
  myGLCD.clrScr();
  myGLCD.fillScr(224, 224, 224);

  myGLCD.setColor(255, 0, 0);

//  Serial.println("DHT11 TEST PROGRAM ");
//  Serial.print("LIBRARY VERSION: ");
//  Serial.println(DHT11::VERSION);
//  Serial.println();
  myGLCD.print("DHT11 Test Program",CENTER,0);
  myGLCD.print("Library version",LEFT,12);
  myGLCD.print(DHT11LIB_VERSION,RIGHT,12);
  delay(2000);
}


void loop()
{
  tempUpdate();
  delay(2000);
}

void tempUpdate()
{
  int temp=0;
  int decimal;
  float meanTemp;
  int chk = DHT11.read(DHT11PIN);
  if (chk==DHTLIB_OK)
  {
    tempbuf[myloop]=(byte)DHT11.temperature;
    myGLCD.printNumI((float)DHT11.temperature,LEFT,60);
    myGLCD.setFont(SevenSegNumFont);
    myGLCD.printNumI((float)DHT11.humidity,RIGHT,24);
    myGLCD.setFont(SmallFont);
    myGLCD.printNumI((float)myloop,LEFT,24);
    myloop=(++myloop&DMaxBuf1);
    buffull|=(myloop==0);
    for(int i=0;i<DMaxBuf;i++)
    {
       temp+=(int)tempbuf[i];
    }
    meanTemp=(float)temp/(float)DMaxBuf;
    myGLCD.printNumF((float)meanTemp,1,LEFT,36);
    myGLCD.setFont(SevenSegNumFont);
    if (buffull)
    {
      myGLCD.printNumI((float)meanTemp,80,74);
      decimal=(10*meanTemp-10*int(meanTemp));
      myGLCD.setFont(BigFont);
      myGLCD.printNumI((float)decimal,RIGHT,74);
      myGLCD.setFont(SmallFont);
      myGLCD.print("GO   ",LEFT,48);
    }
    else
    {
      myGLCD.printNumI((float)DHT11.temperature,80,74);
      myGLCD.setFont(SmallFont);
      myGLCD.print("Wait ",LEFT,48);
    }
  }
}
