/*
Kod för Klocka SK6812 WWA, SK6812 RGBW, SK6812 Låst
Hårdvara: Melvin Olsson | Mjukvara: William Andersson
Senast ändrad: 2023-07-23 00:05
*/
#include <Arduino.h>
#include <Wire.h>     // Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include "RTClib.h"   //RTC lib Logic & functions Neiron (Inte Adafruit)
#include <FastLED.h> //lib funktions for ws2812b
#include <OneButton.h> //till knappen

//LED inställningar
#define LED_PIN 2
#define NUM_LEDS 154 //inklusive minut
#define LED_MODE 0 // 0 = SK6812 WWA | 1 = SK6812 RGBW | 2 = SK6812 Låst
CRGB ledColors[8];
int ledColorsSize = ((sizeof(ledColors) / sizeof(ledColors[0])) - 1);
int currentColor = constrain(1, 0, ledColorsSize);

//Knapp inställningar
#define BUTTON_PIN 5

OneButton btn = OneButton(
  BUTTON_PIN,  // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

//Program Variablar
CRGB leds[NUM_LEDS];
DS1307 rtc;
const int X = 199; //Placeholder led
DateTime pastTime = DateTime(0,0,0,0,0,0); // Sätter pastTime på noll som start.

int brightness = constrain(180, 5, 255);
bool btnLongPress = false;
bool brightnessUpDown = true; // true = upp
int btnHoldCount = 0;

const int hourLeds[12][8] //ETT till TOLV
{
  /*0 ETT  */ {10,11,28,29,50,51, X,X},
  /*1 TVÅ  */ {12,13,26,27,52,53, X,X},
  /*2 TRE  */ {14,15,24,25,54,55, X,X},
  /*3 FYRA */ {100,101,116,117,130,131,142,143},
  /*4 FEM  */ {114,115,132,133,140,141, X,X},
  /*5 SEX  */ {16,17,22,23,56,57, X,X},
  /*6 SJU  */ {118,119,128,129,144,145, X,X},
  /*7 ÅTTA */ {62,63,78,79,84,85,94,95},
  /*8 NIO  */ {18,19,20,21,58,59, X,X},
  /*9 TIO  */ {120,121,126,127,146,147, X,X},
  /*10 ELVA*/ {60,61,80,81,82,83,96,97},
  /*11 TOLV*/ {98,99,122,123,124,125,148,149},
};

const int wordLeds[12][22] //Över, Halv osv..
{
  /*0 Klockan Är   */ {0,1,38,39,40,41,70,71,72,73,86,87,88,89,106,107,136,137, X,X,X,X},
  /*1 Fem I        */ {2,3,36,37,42,43,110,111, X,X,X,X,X,X,X,X,X,X,X,X,X,X},
  /*2 Tio I        */ {90,91,104,105,108,109,110,111, X,X,X,X,X,X,X,X,X,X,X,X,X,X},
  /*3 Kvart I      */ {4,5,34,35,44,45,68,69,74,75,110,111, X,X,X,X,X,X,X,X,X,X},
  /*4 Tjugo I      */ {6,7,32,33,46,47,66,67,76,77,110,111, X,X,X,X,X,X,X,X,X,X},
  /*5 Fem I Halv   */ {2,3,36,37,42,43,110,111,102,103,112,113,134,135,138,139,X,X,X,X,X,X},
  /*6 Halv         */ {102,103,112,113,134,135,138,139, X,X,X,X,X,X,X,X,X,X,X,X,X,X},
  /*7 Fem Över Halv*/ {2,3,36,37,42,43,8,9,30,31,48,49,64,65,102,103,112,113,134,135,138,139},
  /*8 Fem Över     */ {2,3,36,37,42,43,8,9,30,31,48,49,64,65, X,X,X,X,X,X,X,X},
  /*9 Tio Över     */ {90,91,104,105,108,109,8,9,30,31,48,49,64,65, X,X,X,X,X,X,X,X},
  /*10 Kvart Över  */ {4,5,34,35,44,45,68,69,74,75,8,9,30,31,48,49,64,65, X,X,X,X},
  /*11 Tjugo Över  */ {6,7,32,33,46,47,66,67,76,77,8,9,30,31,48,49,64,65, X,X,X,X},
};

const int minuteLeds[4][4] //Små lamporna som visar minut
{
  /*Minut 1*/ {153, X,X,X},
  /*Minut 2*/ {153,152, X,X},
  /*Minut 3*/ {153,152,151, X},
  /*Minut 4*/ {153,152,151,150},
};

//FUNKTIONER
void setup()
{
  Wire.begin();
  rtc.begin();
  if (!rtc.begin()){} // Om arduino inte kan hitta RTC.
  if (!rtc.isrunning()) {}

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //Uppdatera tid till tiden då senaste gången koden laddades upp


  //Led color

  if(LED_MODE == 0)
  {
    CRGB colors[] = {
      //Amber | Kall vit | Varm vit
      CRGB(0,255,0),
      CRGB(0,255,50),
      CRGB(0,200,100),
      CRGB(0,150,150),
      CRGB(0,100,200),
      CRGB(0,50,255),
      CRGB(0,0,255),
      CRGB(255,0,255),
    };
    for (int i = 0; i < (ledColorsSize + 1); i++)
    {
      ledColors[i] = colors[i];
    }
  }
  else if(LED_MODE != 0)
  {
    currentColor = 1;
    ledColors[currentColor] = CRGB(255,255,255);
  }

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness); // Set initial brightness
  LedsOFF();
  FastLED.show();

  btn.attachClick(btnClick);
  btn.attachLongPressStart(btnLongPressStart);
  btn.attachLongPressStop(btnLongPressStop);
}

void LedsOFF()
{
  for(int led = 0; led < NUM_LEDS; led++)
  {
    leds[led].setRGB(0,0,0);
  }
  FastLED.show();
}

void ChangeTime(int _hour, int _minute)
{
  //Återställ alla lampor
  for(int led = 0; led < NUM_LEDS; led++)
  {
    leds[led].setRGB(0,0,0);
  }
  delay(25);

  //Ta fram hur många kolumner det är i arrays
  int wordLedsCols = (sizeof(wordLeds[0]) / sizeof(wordLeds[0][0]));
  int hourLedsCols = (sizeof(hourLeds[0]) / sizeof(hourLeds[0][0]));
  int minuteLedsCols = (sizeof(minuteLeds[0]) / sizeof(minuteLeds[0][0]));

  //Tänd Klockan är
  for(int i = 0; i < wordLedsCols; i++)
  {
    leds[wordLeds[0][i]] = ledColors[currentColor];
  }
  delay(25);

  //Tänd vilken timme det är

  //Konvertera från digitala timmar till analoga
  if(_hour == 13) { _hour = 1; }
  if(_hour == 14) { _hour = 2; }
  if(_hour == 15) { _hour = 3; }
  if(_hour == 16) { _hour = 4; }
  if(_hour == 17) { _hour = 5; }
  if(_hour == 18) { _hour = 6; }
  if(_hour == 19) { _hour = 7; }
  if(_hour == 20) { _hour = 8; }
  if(_hour == 21) { _hour = 9; }
  if(_hour == 22) { _hour = 10; }
  if(_hour == 23) { _hour = 11; }
  if(_hour == 24 || _hour == 0) { _hour = 12; }

  if(_minute < 25)
  {
    _hour = _hour - 1; //Om klockan är under 25 så ska det stå nuvarande timme.
  }
  else if(_minute >= 25 && _hour == 12)
  {
    _hour = 0;
  }

  for(int i = 0; i < hourLedsCols; i++)
  {
    leds[hourLeds[_hour][i]] = ledColors[currentColor];
  }
  delay(25);

  //Tänd avrundad minut (Fem I, Tio I osv)
  int minuteArrayNR = 0;
  int minuteRounded = 0;
  if(_minute < 5) { minuteRounded = 0; minuteArrayNR = 0; } //-------------------XX:00
  if(_minute >= 5 && _minute < 10) { minuteRounded = 5; minuteArrayNR = 8; } //--XX:05
  if(_minute >= 10 && _minute < 15) { minuteRounded = 10; minuteArrayNR = 9; } // XX:10
  if(_minute >= 15 && _minute < 20) { minuteRounded = 15; minuteArrayNR = 10; } //XX:15
  if(_minute >= 20 && _minute < 25) { minuteRounded = 20; minuteArrayNR = 11; } //XX:20
  if(_minute >= 25 && _minute < 30) { minuteRounded = 25; minuteArrayNR = 5; } // XX:25
  if(_minute >= 30 && _minute < 35) { minuteRounded = 30; minuteArrayNR = 6; } // XX:30
  if(_minute >= 35 && _minute < 40) { minuteRounded = 35; minuteArrayNR = 7; } // XX:35
  if(_minute >= 40 && _minute < 45) { minuteRounded = 40; minuteArrayNR = 4; } // XX:40
  if(_minute >= 45 && _minute < 50) { minuteRounded = 45; minuteArrayNR = 3; } // XX:45
  if(_minute >= 50 && _minute < 55) { minuteRounded = 50; minuteArrayNR = 2; } // XX:50
  if(_minute >= 55) { minuteRounded = 55; minuteArrayNR = 1; } //-----------------XX:55
  
  for(int i = 0; i < wordLedsCols; i++)
  {
    leds[wordLeds[minuteArrayNR][i]] = ledColors[currentColor];
  }
  delay(25);

  //Tänd minut prickar
  if(_minute > 0 && _minute < 5)
  {
    for(int i = 0; i < minuteLedsCols; i++)
    {
      leds[minuteLeds[(_minute - 1)][i]] = ledColors[currentColor];
    }
  }
  else if(_minute > 5)
  {
    int displayMin = 0;
    displayMin = (_minute - minuteRounded);
    displayMin = displayMin - 1;
    if(displayMin >= 0)
    {
      for(int i = 0; i < minuteLedsCols; i++)
      {
        leds[minuteLeds[displayMin][i]] = ledColors[currentColor];
      }
    }
  }
  else
  {
    leds[150].setRGB(0,0,0);
    leds[151].setRGB(0,0,0);
    leds[152].setRGB(0,0,0);
    leds[153].setRGB(0,0,0);
  }
  delay(25);

  //Uppdatera allt
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void btnClick()
{
  if(LED_MODE == 0) //SK6812 WWA
  {
    DateTime rtcTime = rtc.now();
    if(currentColor == ledColorsSize)
    {
      currentColor = 0;
      ChangeTime(rtcTime.hour(), rtcTime.minute());
    }
    else
    {
      currentColor = constrain((currentColor + 1), 0, ledColorsSize);
      ChangeTime(rtcTime.hour(), rtcTime.minute());
    }
  }
}

void btnLongPressStart()
{

  if(brightnessUpDown == true) {  brightnessUpDown = false;  }
  else if(brightnessUpDown == false) {  brightnessUpDown = true;  }

  btnLongPress = true;
}

void btnLongPressStop()
{
  btnLongPress = false;
}

void loop()
{
  delay(50); // Hur snabbt loopen ska ticka.
  btn.tick();

  DateTime rtcTime = rtc.now(); // Plockar nuvarande tid.

  //Kollar om tiden har ändrats och isf uppdatera
  if(pastTime.minute() != rtcTime.minute())
  {
    pastTime = rtcTime;
    ChangeTime(rtcTime.hour(), rtcTime.minute());
  }

  if(btnLongPress)
  {
    if(brightnessUpDown == true)
    {
      brightness = constrain((brightness + 2), 5, 255);
      FastLED.setBrightness(brightness);
      FastLED.show();
    }
    else if(brightnessUpDown == false)
    {
      brightness = constrain((brightness - 2), 5, 255);
      FastLED.setBrightness(brightness);
      FastLED.show();
    }
  }
}

