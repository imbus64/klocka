/*
Kod för Klocka SK6812 RGBW
Hårdvara: Melvin Olsson | Mjukvara: William Andersson
Senast ändrad: 2023-11-3 22:38
*/
/*
!!!KNAPP INSTRUKTIONER!!!
Vanligt klick = Växla färg
Dubbel Klick = Växla mellan sommartid och vintertid
Hålla inne = Ljusstyrka
*/
//Libraries
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>   //by Neiron, version 1.6.3
#include <Adafruit_NeoPixel.h>
#include <OneButton.h> //by Matthias Hertel, version 2.1.0

//LED inställningar
const int LED_PIN = 2; //Pin på arduino
const int NUM_LEDS = 154; //Hur många leds som finns(inkl minut)
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_RGBW + NEO_KHZ800);

//Vilken led som de olika minut leds är
const int minutLedNr1 = 153; //Minut 1
const int minutLedNr2 = 152; //Minut 2
const int minutLedNr3 = 151; //Minut 3
const int minutLedNr4 = 150; //Minut 4

//Olika färglägen.
const int ledColors[5][4]  //Färg steg
{
  /*1*/ { 180, 180, 180, 255 },
  /*2*/ { 180, 150, 120, 255 },
  /*3*/ { 100, 65, 80, 255 },
  /*4*/ { 50, 30, 60, 255 },
  /*5*/ { 0, 0, 0, 255 },
};

int ledColorsSize = ((sizeof(ledColors) / sizeof(ledColors[0])) - 1);
int currentColor = constrain(0, 0, ledColorsSize);
int brightness = constrain(180, 5, 255); // 180

//Inställningar för knappen
const int BUTTON_PIN = 5; //Pin på arduino
OneButton btn = OneButton(
  BUTTON_PIN,  // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

bool btnLongPress = false;
bool brightnessUpDown = true; // true = upp
int btnHoldCount = 0;

//Program variablar
const int X = 199; //Placeholder led
DS1307 rtc;
DateTime pastTime = DateTime(0,0,0,0,0,0); // Sätter pastTime på noll som start.

//LED MATRIS
const int hourLeds[12][8]  //ETT till TOLV
{
  /*0 ETT  */ { 10, 11, 28, 29, 50, 51, X, X },
  /*1 TVÅ  */ { 12, 13, 26, 27, 52, 53, X, X },
  /*2 TRE  */ { 14, 15, 24, 25, 54, 55, X, X },
  /*3 FYRA */ { 100, 101, 116, 117, 130, 131, 142, 143 },
  /*4 FEM  */ { 114, 115, 132, 133, 140, 141, X, X },
  /*5 SEX  */ { 16, 17, 22, 23, 56, 57, X, X },
  /*6 SJU  */ { 118, 119, 128, 129, 144, 145, X, X },
  /*7 ÅTTA */ { 62, 63, 78, 79, 84, 85, 94, 95 },
  /*8 NIO  */ { 18, 19, 20, 21, 58, 59, X, X },
  /*9 TIO  */ { 120, 121, 126, 127, 146, 147, X, X },
  /*10 ELVA*/ { 60, 61, 80, 81, 82, 83, 96, 97 },
  /*11 TOLV*/ { 98, 99, 122, 123, 124, 125, 148, 149 },
};

const int wordLeds[12][22]  //Över, Halv osv..
{
  /*0 Klockan Är   */ { 0, 1, 38, 39, 40, 41, 70, 71, 72, 73, 86, 87, 88, 89, 106, 107, 136, 137, X, X, X, X },
  /*1 Fem I        */ { 2, 3, 36, 37, 42, 43, 110, 111, X, X, X, X, X, X, X, X, X, X, X, X, X, X },
  /*2 Tio I        */ { 90, 91, 104, 105, 108, 109, 110, 111, X, X, X, X, X, X, X, X, X, X, X, X, X, X },
  /*3 Kvart I      */ { 4, 5, 34, 35, 44, 45, 68, 69, 74, 75, 110, 111, X, X, X, X, X, X, X, X, X, X },
  /*4 Tjugo I      */ { 6, 7, 32, 33, 46, 47, 66, 67, 76, 77, 110, 111, X, X, X, X, X, X, X, X, X, X },
  /*5 Fem I Halv   */ { 2, 3, 36, 37, 42, 43, 110, 111, 102, 103, 112, 113, 134, 135, 138, 139, X, X, X, X, X, X },
  /*6 Halv         */ { 102, 103, 112, 113, 134, 135, 138, 139, X, X, X, X, X, X, X, X, X, X, X, X, X, X },
  /*7 Fem Över Halv*/ { 2, 3, 36, 37, 42, 43, 8, 9, 30, 31, 48, 49, 64, 65, 102, 103, 112, 113, 134, 135, 138, 139 },
  /*8 Fem Över     */ { 2, 3, 36, 37, 42, 43, 8, 9, 30, 31, 48, 49, 64, 65, X, X, X, X, X, X, X, X },
  /*9 Tio Över     */ { 90, 91, 104, 105, 108, 109, 8, 9, 30, 31, 48, 49, 64, 65, X, X, X, X, X, X, X, X },
  /*10 Kvart Över  */ { 4, 5, 34, 35, 44, 45, 68, 69, 74, 75, 8, 9, 30, 31, 48, 49, 64, 65, X, X, X, X },
  /*11 Tjugo Över  */ { 6, 7, 32, 33, 46, 47, 66, 67, 76, 77, 8, 9, 30, 31, 48, 49, 64, 65, X, X, X, X },
};

const int minuteLeds[4][4] //Små lamporna som visar minut
{
  /*Minut 1*/ {minutLedNr1, X,X,X},
  /*Minut 2*/ {minutLedNr1,minutLedNr2, X,X},
  /*Minut 3*/ {minutLedNr1,minutLedNr2,minutLedNr3, X},
  /*Minut 4*/ {minutLedNr1,minutLedNr2,minutLedNr3,minutLedNr4},
};

//Sommar eller vintertid
bool isSummerTime = true; //Om det är sommartid när du laddar upp koden så ska det vara true, annars false!
const bool uploadInSummerTime = true ; //Om det är sommartid när du laddar upp koden så ska det vara true, annars false!

//ARDUINO FUNKTIONER
void setup()
{
  //Serial.begin(9600);
  //Stara nödvändiga libs
  Wire.begin();
  delay(50);
  rtc.begin();
  while (!rtc.isrunning()) {
    // Väntar tills RTC är igång
    //Serial.println("rtc not avalible");
    delay(100);
  }
  delay(25);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(brightness);
  delay(25);
  btn.attachClick(btnClick);
  btn.attachLongPressStart(btnLongPressStart);
  btn.attachLongPressStop(btnLongPressStop);
  btn.attachDoubleClick(btnMultiClick);
  delay(25);
  //Allt nödvändigt är igång

  //Uppdatera tid till tiden då senaste gången koden laddades upp.
  //Ladda först upp med det på för att sätta tiden, sen kommentera bort det och ladda upp igen.

  //!!OBS! Om du laddar upp när det är vintertid måste "uploadInSummerTime" och "isSummerTime" som du kan hitta högre upp ändras till false!!
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //Starta klockan
  ClockStart();
  delay(50);
  pastTime = DateTime(0,0,0,0,0,0);
  delay(25);
}

//FUNKTIONER
void LedsOFF()
{
  for(int led = 0; led < strip.numPixels(); led++)
  {
    strip.setPixelColor(led, 0, 0, 0, 0);
  }
  delay(50);
  strip.show();
  delay(50);
}

bool LedsOFF_noshow() {
  for (int led = 0; led < strip.numPixels(); led++) {
    strip.setPixelColor(led, 0, 0, 0, 0);
  }
  delay(50);
  return true;
}

void ClockStart()
{
  if(LedsOFF_noshow() == true)
  {
    delay(25);
    for(int led = 0; led < (floor((strip.numPixels() / 2)) + 10); led++)
    {
      strip.setPixelColor(led, ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
      strip.setPixelColor((strip.numPixels() - led), ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
      delay(5);
      strip.show();
      delay(25);
    }
    delay(25);
    LedsOFF();
    delay(50);
  }
}

void ChangeTime(int _hour, int _minute)
{
  //Återställ alla lampor
  if(LedsOFF_noshow() == true)
  {
    //Ta fram hur många kolumner det är i arrays
    int wordLedsCols = (sizeof(wordLeds[0]) / sizeof(wordLeds[0][0]));
    int hourLedsCols = (sizeof(hourLeds[0]) / sizeof(hourLeds[0][0]));
    int minuteLedsCols = (sizeof(minuteLeds[0]) / sizeof(minuteLeds[0][0]));
    delay(25);

    //Tänd Klockan är
    for(int i = 0; i < wordLedsCols; i++)
    {
      strip.setPixelColor((wordLeds[0][i]), ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
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

    delay(25);

    for(int i = 0; i < hourLedsCols; i++)
    {
      strip.setPixelColor((hourLeds[_hour][i]), ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
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
  
    delay(25);

    for(int i = 0; i < wordLedsCols; i++)
    {
      strip.setPixelColor((wordLeds[minuteArrayNR][i]), ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
    }
    delay(25);

    //Tänd minut prickar
    if(_minute > 0 && _minute < 5)
    {
      for(int i = 0; i < minuteLedsCols; i++)
      {
        strip.setPixelColor((minuteLeds[(_minute - 1)][i]), ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
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
          strip.setPixelColor((minuteLeds[displayMin][i]), ledColors[currentColor][0], ledColors[currentColor][1], ledColors[currentColor][2], strip.gamma8(ledColors[currentColor][3]));
        }
      }
    }
    else
    {
      strip.setPixelColor(minutLedNr1, 0, 0, 0, 0);
      strip.setPixelColor(minutLedNr2, 0, 0, 0, 0);
      strip.setPixelColor(minutLedNr3, 0, 0, 0, 0);
      strip.setPixelColor(minutLedNr4, 0, 0, 0, 0);
    }
    delay(50);

    //Uppdatera allt
    strip.setBrightness(brightness);
    strip.show();
  }
}

void TriggerUpdate()
{
  delay(10);
  if (pastTime.minute() > 0)
  {
    pastTime = DateTime(pastTime.year(), pastTime.month(), pastTime.day(), pastTime.hour(), pastTime.minute() - 1, pastTime.second());
  }
  else
  {
    pastTime = DateTime(pastTime.year(), pastTime.month(), pastTime.day(), pastTime.hour(), pastTime.minute() + 1, pastTime.second());
  }
}

void btnClick()
{
  DateTime rtcTime = rtc.now();
  if(currentColor == ledColorsSize)
  {
    currentColor = 0;
    TriggerUpdate();
  }
  else
  {
    currentColor = constrain((currentColor + 1), 0, ledColorsSize);
    TriggerUpdate();
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

void btnMultiClick()
{
  if(isSummerTime == true) {  isSummerTime = false;  }
  else if(isSummerTime == false) {  isSummerTime = true;  }
  TriggerUpdate(); 
}

void loop()
{
  delay(10); // Hur snabbt loopen ska ticka.
  btn.tick();

  DateTime rtcTime = rtc.now(); // Plockar nuvarande tid.

  //Kollar om tiden har ändrats och isf uppdatera
  if(pastTime.minute() != rtcTime.minute())
  {
    pastTime = rtcTime;
    if(isSummerTime == true)
    {
      //Sommartid
      if(uploadInSummerTime == true)
      {
        //Serial.println("1");
        ChangeTime(rtcTime.hour(), rtcTime.minute());
      }
      else
      {
        //Serial.println("2");
        ChangeTime((rtcTime.hour() + 1), rtcTime.minute());
      }
    }
    else if(isSummerTime == false)
    {
      //Vintertid
      if(uploadInSummerTime == true)
      {
        //Serial.println("3");
        if(rtcTime.hour() == 0)
        {
          ChangeTime(23), rtcTime.minute());
        }
        else
        {
          ChangeTime((rtcTime.hour() - 1), rtcTime.minute());
        }        
      }
      else
      {
        //Serial.println("4");
        ChangeTime(rtcTime.hour(), rtcTime.minute());
      }
    }
  }

  if(btnLongPress)
  {
    if(brightnessUpDown == true)
    {
      brightness = constrain((brightness + 2), 5, 255);
      strip.setBrightness(brightness);
      strip.show();
    }
    else if(brightnessUpDown == false)
    {
      brightness = constrain((brightness - 2), 5, 255);
      strip.setBrightness(brightness);
      strip.show();
    }
  }
}
