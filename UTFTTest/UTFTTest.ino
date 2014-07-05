// UTFT_Demo_160x128_Serial (C)2013 QDtech Co.,LTD
// This program is a demo of how to use most of the functions
// of the library with a supported display modules.
//
// This demo was made for modules with a screen resolution 
// of 160x128 pixels.
//
// This program requires the UTFT library.
//
// Changes by NAD to minimize the memory needed
// Will try to remove parts of teh graphicslibrary in a organized way.


#include <UTFT.h>


//for QD_TFT180X SPI LCD Modle
//http://www.QDtech.net
//http://qdtech.taobao.com
//Param1:Value Can be:QD_TFT180A/QD_TFT180B/QD_TFT180C
//Param2 instructions:Connect to LCD_Pin SDA/SDI/MOSI(it means LCD_Model Pin_SDA/SDI/MOSI Connect to Arduino_UNO Pin11 MOSI)
//Param3 instructions:Connect to LCD_Pin SCL/CLK/SCLK(it means LCD_Model Pin_SCL/CLK/SCLK Connect to Arduino_UNO Pin10 SS)
//Param4 instructions:Connect to LCD_Pin CS/CE(it means LCD_Model Pin_CS/CE Connect to Arduino_UNO Pin9 Data)
//Param5 instructions:Connect to LCD_Pin RST/RESET(it means LCD_Model Pin_RST/RESET Connect to Arduino_UNO Pin12 Reset)
//Param6 instructions:Connect to LCD_Pin RS/DC(it means LCD_Model Pin_RS/DC Connect to Arduino_UNO Pin8 Data)
//UTFT myGLCD(QD_TFT180C,11,10,9,12,8);   // Remember to change the model parameter to suit your display module!
// 14,13,12,11?
UTFT myGLCD(QD_TFT180C,11,10,9,8,7);
// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
void setup()
{
  randomSeed(analogRead(0));
  
// Setup the LCD
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
}

void loop()
{
  int buf[158];
  int x, x2;
  int y, y2;
  int r;

// Clear the screen and draw the frame
  myGLCD.setBackColor(224, 224, 224);
  myGLCD.clrScr();

  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(224, 224, 224);
  myGLCD.fillRect(0, 114, 159, 127);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Universal TFT Lib.", CENTER, 1);
  myGLCD.setBackColor(64, 64, 64);
  myGLCD.setColor(255,255,0);
  myGLCD.print("QDtech Co.LTD", LEFT, 114);
  myGLCD.print("(C)2013", RIGHT, 114);

  myGLCD.setColor(0, 0, 255);
  myGLCD.drawRect(0, 13, 159, 113);
  
// Draw crosshairs
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw crosshairs", CENTER, 1);
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(244, 244, 244);
  myGLCD.drawLine(79, 14, 79, 113);
  myGLCD.drawLine(1, 63, 158, 63);
  
  for (int i=9; i<150; i+=10)
    myGLCD.drawLine(i, 61, i, 65);
  for (int i=19; i<110; i+=10)
    myGLCD.drawLine(77, i, 81, i);

// Draw sin-, cos- and tan-lines  
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("Draw sin/cos/tan.", CENTER, 1);
  myGLCD.setColor(0,255,255);
  myGLCD.print("Sin", 5, 15);
  for (int i=1; i<158; i++)
  {
    myGLCD.drawPixel(i,63+(sin(((i*2.27)*3.14)/180)*40));
  }
  
  myGLCD.setColor(255,0,0);
  myGLCD.print("Cos", 5, 27);
  for (int i=1; i<158; i++)
  {
    myGLCD.drawPixel(i,63+(cos(((i*2.27)*3.14)/180)*40));
  }

  myGLCD.setColor(255,255,0);
  myGLCD.print("Tan", 5, 39);
  for (int i=1; i<158; i++)
  {
    myGLCD.drawPixel(i,63+(tan(((i*2.27)*3.14)/180)));
  }

  delay(2000);

//  myGLCD.setColor(224,224,224);
//  myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(224, 224, 224);
  myGLCD.drawLine(79, 14, 79, 113);
  myGLCD.drawLine(1, 63, 158, 63);

// Draw a moving sinewave
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw moving sinewave", CENTER, 1);
  x=1;
  for (int i=1; i<(158*20); i++) 
  {
    x++;
    if (x==159)
      x=1;
    if (i>159)
    {
      if ((x==79)||(buf[x-1]==63))
        myGLCD.setColor(0,0,255);
      else
        myGLCD.setColor(244,244,244);
      myGLCD.drawPixel(x,buf[x-1]);
    }
    myGLCD.setColor(0,0,255);
    y=63+(sin(((i*2.5)*3.14)/180)*(40-(i / 100)));
    myGLCD.drawPixel(x,y);
    buf[x-1]=y;
  }

  delay(2000);
  
  //myGLCD.setColor(224,224,224);
  //myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  
// Draw some filled rectangles
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw fill rectangles", CENTER, 1);
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillRect(39+(i*10), 23+(i*10), 59+(i*10), 43+(i*10));
  }

  delay(2000);
  
  //myGLCD.setColor(224,224,224);
  //myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  
// Draw some filled, rounded rectangles
// NAD- Remove
/*
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("fill round rectangle", CENTER, 1);
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillRoundRect(99-(i*10), 23+(i*10), 119-(i*10), 43+(i*10));
  }
  
  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,158,113);
*/
// Draw some filled circles
// NAD - Remove
/*
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw fill circles", CENTER, 1);
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillCircle(49+(i*10),33+(i*10), 15);
  }
  
  delay(2000);
*/    
  //myGLCD.setColor(224,224,224);
  //myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  
// Draw some lines in a pattern
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw line in pattern", CENTER, 1);
  myGLCD.setColor (255,0,0);
  for (int i=14; i<113; i+=5)
  {
    myGLCD.drawLine(1, i, (i*1.44)-10, 112);
  }
  myGLCD.setColor (255,0,0);
  for (int i=112; i>15; i-=5)
  {
    myGLCD.drawLine(158, i, (i*1.44)-12, 14);
  }
  myGLCD.setColor (0,255,255);
  for (int i=112; i>15; i-=5)
  {
    myGLCD.drawLine(1, i, 172-(i*1.44), 14);
  }
  myGLCD.setColor (0,255,255);
  for (int i=15; i<112; i+=5)
  {
    myGLCD.drawLine(158, i, 171-(i*1.44), 112);
  }
  
  delay(2000);
  
  //myGLCD.setColor(224,224,224);
  //myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  
// Draw some random circles
// NAD - Remove
/*
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw random circles", CENTER, 1);
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=22+random(116);
    y=35+random(57);
    r=random(20);
    myGLCD.drawCircle(x, y, r);
  }

  delay(2000);
*/  
  //myGLCD.setColor(224,224,224);
  //myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  
// Draw some random rectangles
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw random rectangl", CENTER, 1);
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(156);
    y=16+random(95);
    x2=2+random(156);
    y2=16+random(95);
    myGLCD.drawRect(x, y, x2, y2);
  }

  delay(2000);
  
  //myGLCD.setColor(224,224,224);
  //myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  
// Draw some random rounded rectangles
// NAD - Remove
/*
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw round rectangle", CENTER, 1);
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(156);
    y=16+random(95);
    x2=2+random(156);
    y2=16+random(95);
    myGLCD.drawRoundRect(x, y, x2, y2);
  }

  delay(2000);
*/
 // Draw some random Line
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw random Line", CENTER, 1); 
  myGLCD.setColor(244,244,244);
  myGLCD.fillRect(1,14,158,113);

  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(156);
    y=16+random(95);
    x2=2+random(156);
    y2=16+random(95);
    myGLCD.drawLine(x, y, x2, y2);
  }

  delay(2000);
   // Draw some random point
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Draw random point", CENTER, 1); 
//  myGLCD.setColor(244,244,244);
//  myGLCD.fillRect(1,14,158,113);
  ClearShowArea();
  for (int i=0; i<5000; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    myGLCD.drawPixel(2+random(156), 16+random(95));
  }

  delay(2000);

// Test of fonts....

// Small font
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Small Character test", CENTER, 1); 

  ClearShowArea();
  myGLCD.setBackColor(244, 244, 244);
  myGLCD.setColor(255, 0, 0);
  x=0;
  y=15;
  for(int i=32;i<127;i++)
  {
    myGLCD.printChar(byte(i),x++*8+1,y);
    if (x>18)
    {
      y=y+12;
      x=0;
    }
  }
  delay(2000);

// Big font
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 159, 13);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("Big Character test", CENTER, 1); 

  myGLCD.setFont(BigFont);
  ClearShowArea();
  myGLCD.setBackColor(244, 244, 244);
  myGLCD.setColor(255, 0, 0);
  x=0;
  y=15;
  for(int i=32;i<127;i++)
  {
    myGLCD.printChar(byte(i),x++*16+1,y);
    if (x>8)
    {
      y=y+16;
      x=0;
      if (y+16>113)
      {
        y=15;
        delay(1000);
        ClearShowArea();
        myGLCD.setColor(255, 0, 0);
      }
    }
  }
  myGLCD.setFont(SmallFont);
  delay(2000);
  
  //Game Over!
  myGLCD.fillScr(0, 0, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(10, 17, 149, 72);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("That's it!", CENTER, 20);
  myGLCD.print("Restarting in a", CENTER, 45);
  myGLCD.print("few seconds...", CENTER, 57);
  
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("Runtime: (msecs)", CENTER, 103);
  myGLCD.printNumI(millis(), CENTER, 115);

  delay (10000);
}
void ClearShowArea()
{
  myGLCD.setColor(244,244,244);
  myGLCD.fillRect(1,14,158,113);
  
}

