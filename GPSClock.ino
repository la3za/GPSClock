
// Set version and date manually for code status display
const char codeVersion[]   = "v1.5.0     3.11.2022";

// or set date automatically to ompilation date (US format) - nice to use during development - while version number is set manually
//const char codeVersion[] = "v1.4.0   "__DATE__;  

/*
  LA3ZA Multi Face GPS Clock

  Copyright 2015 - 2022 Sverre Holm, LA3ZA
  All trademarks referred to in source code and documentation are copyright their respective owners.


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

  If you offer a hardware kit using this software, show your appreciation by sending the author a complimentary kit ;-)

  Full documentation can be found at https://github.com/la3za .  Please read it before requesting help.


  Features:
           GPS clock on 20x4 I2C LCD
           Controlled by a GPS module outputting data over a serial interface, typ. QRPLabs QLG1 GPS Receiver kit, or the QLG2
           GPS is handled with the TinyGPS++ library
           Shows raw GPS data such as UTC time and date, position, altitude, and number of satellitess
           Also with various forms of binary, BCD, hex and octal displays
           Shows derived GPS data such as 6-digit locator
           Finds local time and handles daylight saving automatically using the Timezone library
           Finds local sunset and sunrise, either actual value, or civil, nautical, or astronomical using the Sunrise library
           The clock also gives local solar height based on the Sunpos library from the K3NG rotator controller.
           The clock also provides the lunar phase as well as predicts next rise/set time for the moon


           Input   from GPS
           Output:
           UTC time with day
           Local time with day
           Automatic daylight saving for local time
           Longitude, latitude, altitude
           Locator, e.g. JO59fu
           Number of satellites used for position
             Moon phase, rise/set times/azimuth
           Solar data:
             Actual, Civil rise and set times
             Time of solar noon
             Solar elevation
             PWM control of LCD backlight via potentiometer on analog input
*/

/* Functions in this file - one per screen type:

            setup
            loop

            LocalUTC
            UTCLocator

            LocalSun
            LocalSunAzEl [added 12.11.2021]
            LocalSunMoon
            LocalMoon
            MoonRiseSet

            Binary
            Bar
            MengenLehrUhr
            LinearUhr
            Hex
            Octal

            InternalTime
            CodeStatus

            UTCPosition

            NCDXFBeacons
            WSPRsequence

            HexOctalClock 
            EasterDates   
            MathClock     
            LunarEclipse   
                                   
            Roman         
            Morse         
            WordClock    
            Sidereal      
            DemoClock 

            PlanetVisibility
            ISOHebIslam
            GPSInfo     
             
            

/*

  Revisions:

 1.5.0  xx.11.2022:
                - PlanetVisibility
                - ISOHebIslam
                - GPSInfo
                - Option for showing local week number (ISO) rather than locator in local time display 
                - cycleTime in UTCPosition for cycling between different formats for position increased from 4 to 10 sec
                - fixed bug in menu system for menu # 0, now initialized to menuOrder[iiii] = -1, rather than to 0.

 1.4.0  25.07.2022:
                - Multiple language support: 'no', 'se', 'dk', 'is', 'de', 'fr', 'es' day names for local time in addition to English
                - Cleaned display from LcdSolarRiseSet for Northern positions in case sun never goes below Civil, Nautical, Astronomical limit in summer
                - Follows this naming conventions better:
                -- function:  CapitalLetterFirst      
                -- variable:  smallLetterFirst
                -- defines:   CAPITAL_LETTERS
                

 1.3.0 05.04.2022:
                - WordClock - displays time in words
                - DemoClock - cycles through all the other selected modes
                - Sidereal  - Local Sidereal and Solar Time
                - Morse     - Morse display on lcd
                - Roman     - Roman number clock 

          
          1.2.1 07.03.2022:
                - Removed unused code WordClock
                - Fixed display of short day names in Native Language (affects Norwegian only)                                

          1.2.0 21.01.2022
                - MathClock - 4 ways to present an arithmetic computation to find hour and minute at regular intervals
                - LunarEclipse - predict lunar eclipses 2-3 years into the future
                - MoonRiseSet now only shows future events
                - New LocalSunAzEl, 
                - New LocalSun(1) with a simpler look (LocalSun(0) is the old look)
                - HexOctalClock(3) now always shows normal decimal clock also
                - Menu system simplified by using descriptive names for screens not just numbers (see definitions in clock_defines.h)

          1.1.1 14.11.2021
                - Fixed bug in selection of favorite menu item
                
          1.1.0 25.10.2021
                - Implemented rotary encoder for control of screen number as an alternative (or in addition to) up/down buttons
                - FEATURE_PUSH_FAVORITE - push on rotary encoder: jump to favorite screen. If not set: adjust backlight
                - New defines FEATURE_POTENTIOMETER, FEATURE_BUTTONS, FEATURE_ROTARY_ENCODER, FEATURE_PUSH_FAVORITE

          1.0.4  18.10.2021
                - Fixed small formatting bug in LcdShortDayDateTimeLocal which affected display of date on line 0 in several screens
                - Added screen 22, EasterDates, with dates for Gregorian and Julian Easter Sunday three years ahead

		      1.0.3  11.10.2021
                - Added missing 6. bit in minutes, seconds in binary clocks
		            - Added screen 21 with simultaneous binary, octal, and hex clocks
                - Removed FEATURE_CLOCK_SOME_SECONDS, replaced by SECONDS_CLOCK_HELP = (0...60) for additional "normal" clock in binary, octal, BCD, etc

          1.0.2  06.10.2021
                - UTC and position screen: Changed layout. Now handles Western longitudes and Southern latitudes also. Thanks Mitch W4OA
                - Corrected bug in Maidenhead routine, only appeared if letter 5 was beyond a certain letter in the alphabet. Thanks Ross VA1KAY

          1.0.1  29.09.2021
                - Fixed  small layout bug on screen
                - New variable in clock_options.h: SECONDS_CLOCK_HELP - no of seconds to show normal clock in binary, BCD, hex, octal etc clocks.
                - 2 new screens: no 19 and 20: hex and octal clock

          1.0.0  24.09.2021
                - First public release
                - 18 different screens


*/

///// load user-defined setups //////
#include "clock_defines.h"

#include "clock_debug.h"            // debugging options via serial port
#include "clock_options.h"        // customization of order and number of menu items
#include "clock_pin_settings.h"     // hardware pins 

//////////////////////////////////////

// libraries

#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time - timekeepng functionality
#include <Timezone_Generic.h>   // https://github.com/khoih-prog/Timezone_Generic

#include "clock_zone.h"         // user-defined setup for local time zone and daylight saving

#include <TinyGPS++.h>          // http://arduiniana.org/libraries/tinygpsplus/

#include "clock_planets.h"
#include "clock_calendar.h"
char buffer[80];     // the code uses 70 characters. For display strings in calendar display
static tmElements_t curr_time;

#if defined(FEATURE_LCD_I2C)
#include <Wire.h>               // For I2C. Comes with Arduino IDE
#include <LiquidCrystal_I2C.h>  // Install NewliquidCrystal_1.3.4.zip https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/
#endif

#if defined(FEATURE_LCD_4BIT)
#include <LiquidCrystal.h>
#endif

#ifdef FEATURE_ROTARY_ENCODER
//  #define HALF_STEP                 // not needed
#include <rotary.h>                 // rotary handler https://bitbucket.org/Dershum/rotary_button/src/master/
#endif

#include <Sunrise.h>            // https://github.com/chaeplin/Sunrise, http://www.andregoncalves.info/ag_blog/?p=47
// Now in AVR-Libc version 1.8.1, Aug. 2014 (not in Arduino official release)

// K3NG https://blog.radioartisan.com/yaesu-rotator-computer-serial-interface/
//      https://github.com/k3ng/k3ng_rotator_controller
#include <sunpos.h>      // http://www.psa.es/sdg/archive/SunPos.cpp (via https://github.com/k3ng/k3ng_rotator_controller/tree/master/libraries)
#include <moon2.h>       // via https://github.com/k3ng/k3ng_rotator_controller/tree/master/libraries


#if defined(FEATURE_LCD_I2C)
//            set the LCD address to 0x27 and set the pins on the I2C chip used for LCD connections:
//                     addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
  LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
#endif


#if defined(FEATURE_LCD_4BIT)
  LiquidCrystal lcd(lcd_rs, lcd_enable, lcd_d4, lcd_d5, lcd_d6, lcd_d7);
#endif

#ifdef FEATURE_ROTARY_ENCODER
// Initialize the Rotary object
  Rotary r = Rotary(PINA, PINB, PUSHB);
  int toggleRotary = 1;
#endif

#define RAD                 (PI/180.0)
#define SMALL_FLOAT         (1e-12)

// LCD characters:
#define DASHED_UP_ARROW        1 // user defined character
#define DASHED_DOWN_ARROW      2 // user defined character
#define UP_ARROW               3 // user defined character
#define DOWN_ARROW             4 // user defined character
#define APOSTROPHE            34 // in LCD character set
#define BIG_DOT              165 // in LCD character set
#define THREE_LINES          208 // in LCD character set
#define SQUARE               219 // in LCD character set
#define DEGREE               223 // in LCD character set
#define ALL_OFF              254 // in LCD character set
#define ALL_ON               255 // in LCD character set

int dispState ;  // depends on button, decides what to display
int old_dispState;
int demoDispState = -2; // ensures Demo Mode starts by displaying D E M O
char today[12];
char todayFormatted[12];
double latitude, lon, alt;
int Year;
byte Month, Day, Hour, Minute, Seconds;
u32 noSats;

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local; 
time_t oldNow = 0;
time_t prevDisplay = 0;     // when the digital clock was displayed

int packedRise;
double moon_azimuth = 0;
double moon_elevation = 0;
double moon_dist = 0;

int iiii;
int oldMinute = -1;

int yearGPS;
uint8_t monthGPS, dayGPS, hourGPS, minuteGPS, secondGPS, weekdayGPS;

//int backlightVal; // value which controls backlight brighness
int menuOrder[50]; //menuOrder[noOfStates]; // must be large enough to hold all possible screens!!


/*
  Uses Serial1 for GPS input
  4800; // OK for EM-406A and ADS-GM1
  9600; // OK for NEO-6M
  Serial1 <=> pin 19 on Mega
*/
TinyGPSPlus gps; // The TinyGPS++ object

#include "clock_lunarCycle.h"
#include "clock_custom_routines.h"  // user customable functions for local language
#include "clock_helper_routines.h"  // library of functions

#include "clock_moon_eclipse.h"
#include "clock_equatio.h"

 // builds on the example program SatelliteTracker from the TinyGPS++ library
  // https://www.arduino.cc/reference/en/libraries/tinygps/
  /*
    From http://aprs.gids.nl/nmea/:
   
  $GPGSV
  
  GPS Satellites in view (SV - Satellite in View)
  
  eg. $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
      $GPGSV,3,2,11,14,25,170,00,16,57,208,39,18,67,296,40,19,40,246,00*74
      $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D

  1    = Total number of messages of this type in this cycle
  2    = Message number
  3    = Total number of SVs in view
  4    = SV PRN number
  5    = Elevation in degrees, 90 maximum
  6    = Azimuth, degrees from true north, 000 to 359
  7    = SNR, 00-99 dB (null when not tracking)
  8-11 = Information about second SV, same as field 4-7
  12-15= Information about third SV, same as field 4-7
  16-19= Information about fourth SV, same as field 4-7
*/
  static const int MAX_SATELLITES = 40;

  TinyGPSCustom totalGPGSVMessages(gps, "GPGSV", 1); // $GPGSV sentence, first element
  TinyGPSCustom messageNumber(gps, "GPGSV", 2);      // $GPGSV sentence, second element
  TinyGPSCustom satsInView(gps, "GPGSV", 3);         // $GPGSV sentence, third element
  TinyGPSCustom GPSMode(gps, "GPGSA", 2);            // $GPGSA sentence, 2nd element 1-none, 2=2D, 3=3D
  TinyGPSCustom posStatus(gps, "GPRMC", 2);          // $GPRMC sentence: Position status (A = data valid, V = data invalid)
  TinyGPSCustom satNumber[4]; // to be initialized later
  TinyGPSCustom elevation[4];
  TinyGPSCustom azimuth[4];
  TinyGPSCustom snr[4];

  struct
{
  bool active;
  int elevation;
  int azimuth;
  int snr;
} sats[MAX_SATELLITES];


//#include "clock_development.h"

////////////////////////////////////////////////////////////////////////////////
void setup() {
  dispState = 0;
  lcd.begin(20, 4);

  // Store bit maps, designed using editor at https://maxpromer.github.io/LCD-Character-Creator/
  byte upDashedArray[8] = {0x4, 0xa, 0x15, 0x0, 0x4, 0x0, 0x4, 0x0};
  byte downDashedArray[8] = {0x4, 0x0, 0x4, 0x0, 0x15, 0xa, 0x4, 0x0};
  byte upArray[8] = {0x4, 0xe, 0x15, 0x4, 0x4, 0x4, 0x4, 0x0};
  byte downArray[8] = {0x4, 0x4, 0x4, 0x4, 0x15, 0xe, 0x4, 0x0};

  // upload characters to the lcd
  lcd.createChar(DASHED_UP_ARROW, upDashedArray);
  lcd.createChar(DASHED_DOWN_ARROW, downDashedArray);
  lcd.createChar(UP_ARROW, upArray);
  lcd.createChar(DOWN_ARROW, downArray);

#if FEATURE_NATIVE_LANGUAGE == 'no' | FEATURE_NATIVE_LANGUAGE == 'se' | FEATURE_NATIVE_LANGUAGE == 'dk'| FEATURE_NATIVE_LANGUAGE == 'es'| FEATURE_NATIVE_LANGUAGE == 'is'
  #include "clock_native.h"  // user customable character set
#endif

  lcd.clear(); // in order to set the LCD back to the proper memory mode after custom characters have been created

  pinMode(LCD_PWM, OUTPUT);
  digitalWrite(LCD_PWM, HIGH);   // sets the backlight LED to full

#ifdef FEATURE_ROTARY_ENCODER
  digitalWrite (PINA, HIGH);     // enable pull-ups
  digitalWrite (PINB, HIGH);
  digitalWrite (PUSHB, HIGH);
#endif

  // initialize and unroll menu system order
  for (iiii = 0; iiii < sizeof(menuOrder)/sizeof(menuOrder[0]); iiii += 1) menuOrder[iiii] = -1; // fix 5.10.2022
  for (iiii = 0; iiii < noOfStates; iiii += 1) menuOrder[menuIn[iiii]] = iiii;

  CodeStatus();  // start screen
  lcd.setCursor(10, 2); lcd.print("......"); // ... waiting for GPS
  lcd.setCursor(0, 3); lcd.print("        ");
  delay(1000);

#ifndef FEATURE_PC_SERIAL_GPS_IN
  Serial1.begin(gpsBaud);
#else
  Serial.begin(gpsBaud);  // for faking GPS data from software simulator
#endif

  oldNow = now();  

  // Serial output is only used for debugging:

#ifdef FEATURE_SERIAL_SOLAR
  Serial.begin(115200);
  Serial.println(F("Solar debug"));
#endif

#ifdef FEATURE_SERIAL_GPS
  // set the data rate for the Serial port
  Serial.begin(115200);
  Serial.print(F("Testing TinyGPS++ library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();
#endif

#ifdef FEATURE_SERIAL_MOON
  Serial.begin(115200);
  Serial.println(F("Moon debug"));
#endif

#ifdef FEATURE_SERIAL_MENU
  Serial.begin(115200);
  Serial.println(F("menuOrder: "));
  for (iiii = 0; iiii < noOfStates; iiii += 1) Serial.println(menuOrder[iiii]);
#endif

#ifdef FEATURE_SERIAL_TIME
  Serial.begin(115200);
  Serial.println(F("Time debug"));
#endif

#ifdef FEATURE_SERIAL_MATH
  Serial.begin(115200);
  Serial.println(F("Math debug"));
#endif

#ifdef FEATURE_SERIAL_LUNARECLIPSE
  Serial.begin(115200);
  Serial.println(F("Moon eclipse debug"));
#endif

#ifdef FEATURE_SERIAL_EQUATIO
  Serial.begin(115200);
  Serial.println(F("Equation of Time debug"));
#endif


   // from SatelliteTracker (TinyGPS++ example)
   // Initialize all the uninitialized TinyGPSCustom objects
    for (int i=0; i<4; ++i)
    {
      satNumber[i].begin(gps, "GPGSV", 4 + 4 * i); // offsets 4, 8, 12, 16
      elevation[i].begin(gps, "GPGSV", 5 + 4 * i); // offsets 5, 9, 13, 17
      azimuth[i].begin(  gps, "GPGSV", 6 + 4 * i); // offsets 6, 10, 14, 18
      snr[i].begin(      gps, "GPGSV", 7 + 4 * i); // offsets 7, 11, 15, 19
    }
 

}

////////////////////////////////////// L O O P //////////////////////////////////////////////////////////////////
void loop() {

#ifdef FEATURE_POTENTIOMETER
  backlightVal = analogRead(potentiometer);   // read the input pin for control of backlight
  backlightVal = max(backlightVal, 1); // to ensure there is some visibility
  // analogRead values go from 0 to 1023, analogWrite values from 0 to 255

#endif
  // compress using sqrt to get smoother characteristics for the eyes
  analogWrite(LCD_PWM, (int)(255 * sqrt((float)backlightVal / 1023))); //


#ifdef FEATURE_BUTTONS // at least one of FEATURE_BUTTONS or FEATURE_ROTARY_ENCODER must be defined
  byte button = AnalogButtonRead(0); // using K3NG function
  if (button == 2) { // increase menu # by one
    dispState = (dispState + 1) % noOfStates;
    lcd.clear();
    oldMinute = -1; // to get immediate display of some info
    lcd.setCursor(18, 3); lcd.print(dispState); // lower left-hand corner
    delay(300); // was 300
  }
  else if (button == 1) { // decrease menu # by one
    dispState = (dispState - 1) % noOfStates;;
    lcd.clear();
    oldMinute = -1; // to get immediate display of some info
    if (dispState < 0) dispState += noOfStates;
    lcd.setCursor(18, 3); lcd.print(dispState);
    delay(300);
  }
#endif // FEATURE_BUTTONS 

#ifdef FEATURE_ROTARY_ENCODER // one or both of FEATURE_ROTARY_ENCODER or FEATURE_BUTTONS must be defined

    volatile unsigned char result = r.process();
  
    if (r.buttonPressedReleased(25)) {
      toggleRotary = (toggleRotary + 1) % 2;
  #ifdef FEATURE_SERIAL_MENU
      Serial.print("toggleRotary "); Serial.println(toggleRotary);
  #endif
  
  #ifdef FEATURE_PUSH_FAVORITE    // toggle between present dispState and favorite
      if (toggleRotary == 0) {
        old_dispState = dispState;
        dispState = menuOrder[menuFavorite];  // fix 14.11.2021
      }
      else dispState = old_dispState;  // store away previous state when going to favorite, in order to return to it on next press
  
      lcd.clear();
      oldMinute = -1; // to get immediate display of some info
      lcd.setCursor(18, 3); lcd.print(dispState); // lower left-hand corner
      delay(50);
  #endif
  
    } //endif buttonPressedReleased
  
  #ifndef FEATURE_PUSH_FAVORITE             // change backlight value
    if (result && toggleRotary == 0) {      // reduce backlight value
      if (result == DIR_CCW) {
        backlightVal = (backlightVal - 12);
        backlightVal = max(backlightVal, 1);
      }
      else {                                // increase backlight value
        backlightVal = (backlightVal + 12);
        backlightVal = min(backlightVal, 1023);
      }
    }
  #endif
  
  #ifdef FEATURE_POTENTIOMETER
    #ifdef FEATURE_SERIAL_MENU
      Serial.print("backlightVal (backlight) "); Serial.println(backlightVal);
    #endif
  #endif
  
  
    if (result) { // change menu number by rotation
  #ifndef FEATURE_PUSH_FAVORITE
      if (toggleRotary == 1)
  #endif
      {
        if (result == DIR_CCW) {
          dispState = (dispState - 1) % noOfStates;
          lcd.clear();
          oldMinute = -1; // to get immediate display of some info
          if (dispState < 0) dispState += noOfStates;
          lcd.setCursor(18, 3); lcd.print(dispState); // lower left-hand corner
          if (dispState == menuOrder[ScreenDemoClock]) demoDispState = -2;
          delay(50);
        }
        else
        {
          dispState = (dispState + 1) % noOfStates;
          lcd.clear();
          oldMinute = -1; // to get immediate display of some info
          lcd.setCursor(18, 3); lcd.print(dispState); // lower left-hand corner
          if (dispState == menuOrder[ScreenDemoClock]) demoDispState = -2;
          delay(50);
  
        }
      }
    }


#endif // FEATURE_ROTARY_ENCODER

  else {

#ifndef FEATURE_PC_SERIAL_GPS_IN
    while (Serial1.available()) {
      if (gps.encode(Serial1.read())) { // process gps messages from hw GPS
#else
    while (Serial.available()) {
      if (gps.encode(Serial.read())) { // process gps messages from sw GPS emulator
#endif

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Necessary for initializing GPSInfo() properly 22.09.2022:

    if (totalGPGSVMessages.isUpdated())
    {
      for (int i=0; i<4; ++i)
      {
        int no = atoi(satNumber[i].value());
        // Serial.print(F("SatNumber is ")); Serial.println(no);
        if (no >= 1 && no <= MAX_SATELLITES)
        {
          sats[no-1].elevation = atoi(elevation[i].value());
          sats[no-1].azimuth = atoi(azimuth[i].value());
          sats[no-1].snr = atoi(snr[i].value());
          sats[no-1].active = true;
        }
      }
    }
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


        // when GPS reports new data...
        unsigned long age;

        hourGPS = gps.time.hour();
        minuteGPS = gps.time.minute();
        secondGPS = gps.time.second();
        dayGPS = gps.date.day() ;
        monthGPS = gps.date.month() ;
        yearGPS = gps.date.year() ;
        age = gps.location.age();            // age in millisecs (?)

        //int utc, int local; // must be time_t and global
        utc = now(); //+(long)86400*150;

#ifdef AUTO_UTC_OFFSET
    #include "clock_zone2.h" // this file contains the concrete time zone call
        utcOffset = local / long(60) - utc / long(60); // order of calculation is important
#else
        local = utc + utcOffset * 60; // utcOffset in minutes is set manually in clock_zone.h
#endif

#ifdef FEATURE_SERIAL_TIME
        Serial.print(F("utc     ")); Serial.println(utc);
        Serial.print(F("local   ")); Serial.println(local);
        Serial.print(F("diff,s  ")); Serial.println(long(local - utc));
        Serial.print(F("diff,m1 ")); Serial.println(long(local - utc) / long(60));
        Serial.print(F("diff,m  ")); Serial.println(long(local) / long(60) - long(utc) / long(60));

        Serial.print(F("utcOffset: "));
        Serial.println(utcOffset);
        Serial.print("The time zone is: "); Serial.println(tcr -> abbrev);
#endif

        if (age < 500) {
          // set the Time to the latest GPS reading
          setTime(hourGPS, minuteGPS, secondGPS, dayGPS, monthGPS, yearGPS);
          weekdayGPS = weekday();
          // Versions from 17.04.2020 has Arduino time = utc

        }
      }
    }
    if (timeStatus() != timeNotSet) {
      if (now() != prevDisplay) { //update the display only if the time has changed. i.e. every second
        prevDisplay = now();

// this is for jumping from screen to screen in demo Mode:
        if (now() > oldNow + DWELL_TIME_DEMO)
        {
          #ifdef FEATURE_SERIAL_MENU
              Serial.print("demoDispState "); Serial.println(demoDispState);
              Serial.print("dispState "); Serial.print(dispState);Serial.print(" ");Serial.println(menuOrder[ScreenDemoClock]);
          #endif
          demoDispState += 1;
          if (demoDispState < 0) demoDispState += noOfStates;
          demoDispState = demoDispState%noOfStates; 
          oldNow = now(); 
          oldMinute = -1; // to get immediate display of some info
          if (dispState == menuOrder[ScreenDemoClock]) lcd.clear();
        }  

        /////////////////////////////////////////////// USER INTERFACE /////////////////////////////////////////////////////////
#ifdef FEATURE_SERIAL_MENU
 //       Serial.println(F("menuOrder: "));
//        for (iiii = 0; iiii < noOfStates; iiii += 1) Serial.println(menuOrder[iiii]);
        Serial.print(F("dispState ")); Serial.println(dispState);
        Serial.println((dispState % noOfStates));
        Serial.println(menuOrder[dispState % noOfStates]);
#endif

        ////////////// Menu system ////////////////////////////////////////////////////////////////////////////////
        ////////////// This is the order of the menu system unless menuOrder[] contains information to the contrary

        menuSystem(dispState, 0);

      }  // if (now() != prevDisplay) 
    }    // if (timeStatus() != timeNotSet)
  }
}  // end loop


////////////////////////////////////// END LOOP //////////////////////////////////////////////////////////////////


void menuSystem(int dispState, int DemoMode) // menu System - called at end of loop and from DemoClock
{
        if      ((dispState) == menuOrder[ScreenLocalUTC ])     LocalUTC(0);     // local time, date; UTC, locator
        else if ((dispState) == menuOrder[ScreenLocalUTCWeek])  LocalUTC(1);     // local time, date; UTC, week #
        else if ((dispState) == menuOrder[ScreenUTCLocator])    UTCLocator();    // UTC, locator, # sats

        // Sun, moon:
        else if ((dispState) == menuOrder[ScreenLocalSun])      LocalSun(0);      // local time, sun x 3
        else if ((dispState) == menuOrder[ScreenLocalSunMoon])  LocalSunMoon();  // local time, sun, moon
        else if ((dispState) == menuOrder[ScreenLocalMoon])     LocalMoon();     // local time, moon size and elevation
        else if ((dispState) == menuOrder[ScreenMoonRiseSet])   MoonRiseSet();   // Moon rises and sets at these times

        // Nice to have
        else if ((dispState) == menuOrder[ScreenTimeZones])     TimeZones();     // Other time zones

        // Fancy, sometimes near unreadable displays, fun to program, and fun to look at:
        else if ((dispState) == menuOrder[ScreenBinary])        Binary(2);       // Binary, horizontal, display of time
        else if ((dispState) == menuOrder[ScreenBinaryHorBCD])  Binary(1);       // BCD, horizontal, display of time
        else if ((dispState) == menuOrder[ScreenBinaryVertBCD]) Binary(0);       // BCD vertical display of time
        else if ((dispState) == menuOrder[ScreenBar])           Bar();           // horizontal bar
        else if ((dispState) == menuOrder[ScreenMengenLehrUhr]) MengenLehrUhr(); // set theory clock
        else if ((dispState) == menuOrder[ScreenLinearUhr])     LinearUhr();     // Linear clock
        // debugging:
        else if ((dispState) == menuOrder[ScreenInternalTime])  InternalTime();  // Internal time - for debugging
        else if ((dispState) == menuOrder[ScreenCodeStatus])   CodeStatus();   //

        // Nice to have:
        else if ((dispState) == menuOrder[ScreenUTCPosition])   UTCPosition();   // position

        // WSPR and beacons:
        else if ((dispState) == menuOrder[ScreenNCDXFBeacons2]) NCDXFBeacons(2); // UTC + NCDXF beacons, 18-28 MHz
        else if ((dispState) == menuOrder[ScreenNCDXFBeacons1]) NCDXFBeacons(1); // UTC + NCDXF beacons, 14-21 MHz
        else if ((dispState) == menuOrder[ScreenWSPRsequence])  WSPRsequence();  // UTC + Coordinated WSPR band/frequency (20 min cycle)

        else if ((dispState) == menuOrder[ScreenHex])           HexOctalClock(0); // Hex clock
        else if ((dispState) == menuOrder[ScreenOctal])         HexOctalClock(1); // Octal clock
        else if ((dispState) == menuOrder[ScreenHexOctalClock]) HexOctalClock(3); // 3-in-1: Hex-Octal-Binary clock

        else if ((dispState) == menuOrder[ScreenEasterDates])   EasterDates(yearGPS); // Gregorian and Julian Easter Sunday

        else if ((dispState) == menuOrder[ScreenLocalSunSimpler]) LocalSun(1);    // local time, sun x 3 - simpler layout
        else if ((dispState) == menuOrder[ScreenLocalSunAzEl]) LocalSunAzEl();    // local time, sun az, el
        
        else if ((dispState) == menuOrder[ScreenRoman])         Roman();         // Local time in Roman numerals
        else if ((dispState) == menuOrder[ScreenMathClockAdd])  MathClock(0);    // Math clock: add
        else if ((dispState) == menuOrder[ScreenMathClockSubtract]) MathClock(1); // Math clock: subtract/add
        else if ((dispState) == menuOrder[ScreenMathClockMultiply]) MathClock(2); // Math clock: multiply/subtract/add
        else if ((dispState) == menuOrder[ScreenMathClockDivide]) MathClock(3);  // Math clock: divide/multiply/subtract
        
        else if ((dispState) == menuOrder[ScreenLunarEclipse])  LunarEclipse();  // time for lunar eclipse
        else if ((dispState) == menuOrder[ScreenSidereal])      Sidereal();      // sidereal and solar time
        else if ((dispState) == menuOrder[ScreenMorse])         Morse();         // morse time
        else if ((dispState) == menuOrder[ScreenWordClock])     WordClock();     // time in clear text

        else if ((dispState) == menuOrder[ScreenGPSInfo])       GPSInfo();       // Show technical GPS Info
        else if ((dispState) == menuOrder[ScreenISOHebIslam])   ISOHebIslam();   // ISO, Hebrew, Islamic calendar
        else if ((dispState) == menuOrder[ScreenPlanetsInner])  PlanetVisibility(1); // Inner planet data
        else if ((dispState) == menuOrder[ScreenPlanetsOuter])  PlanetVisibility(0); // Inner planet data
        
        else if ((dispState) == menuOrder[ScreenDemoClock])                      // last menu item
        {
           if (!DemoMode) DemoClock(0);       // Start demo of all clock functions if not already in DemoMode
           else           DemoClock(1);
        }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

// The rest of this file consists of one routine per menu item:

// Menu item ///////////////////////////////////////////////////////////////////////////////////////////
void LocalUTC(           // local time, UTC, locator, option: ISO week #
                int mode // 0 for original version
                         // 1 for added week # (new 3.9.2022)
) { // 

  char textBuffer[11];
  // get local time

#ifndef FEATURE_DATE_PER_SECOND 
  local = now() + utcOffset * 60;

#else                             // for stepping date quickly and check calender function
  local = now() + utcOffset * 60 + dateIteration*86400; //int(86400.0/5.0); // fake local time by stepping per day
  dateIteration = dateIteration + 1;
//  Serial.print(dateIteration); Serial.print(": ");
//  Serial.println(local);
#endif
  
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  lcd.setCursor(0, 0); // top line *********
  sprintf(textBuffer, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
  lcd.print(textBuffer);
  lcd.print("      ");
  // local date
  Day = day(local);
  Month = month(local);
  Year = year(local);
  if (dayGPS != 0)
  {

// right-adjusted long day name:
 
#if FEATURE_NATIVE_LANGUAGE == 'no' | FEATURE_NATIVE_LANGUAGE == 'se' | FEATURE_NATIVE_LANGUAGE == 'dk' | FEATURE_NATIVE_LANGUAGE == 'is'
    nativeDayLong(local);
    sprintf(todayFormatted,"%12s", today);

#elif FEATURE_NATIVE_LANGUAGE == 'fr'| FEATURE_NATIVE_LANGUAGE == 'es'|FEATURE_NATIVE_LANGUAGE == 'de' 
    nativeDayLong(local);
    sprintf(todayFormatted,"%12s", today);

#else // English
#ifdef FEATURE_DAY_PER_SECOND
//      fake the day -- for testing only
        sprintf(todayFormatted, "%12s", dayStr( 1+(local/2)%7 )); // change every two seconds
#else
        sprintf(todayFormatted, "%12s", dayStr(weekday(local)));
#endif
 
#endif

    lcd.setCursor(8, 0);
    lcd.print(todayFormatted);


    lcd.setCursor(0, 1); //////// line 2
    if (mode==1)
  {
    // option added 3.9.2022 - ISO week # on second line
        GregorianDate a(month(local), day(local), year(local));      
        IsoDate ISO(a);     
        lcd.print("Week ");lcd.print(ISO.GetWeek());
  }
  else  lcd.print("          ");
    
    lcd.setCursor(10, 1);
    LcdDate(Day, Month, Year);

  }

  lcd.setCursor(0, 2); lcd.print("                    ");
  LcdUTCTimeLocator(3); // / last line *********

}


// Menu item //////////////////////////////////////////////////////////////////////////////////////////

void UTCLocator() {     // UTC, locator, # satellites
  char textBuffer[25];

#ifdef FEATURE_PC_SERIAL_GPS_IN
      hourGPS = hour(now());
      minuteGPS = minute(now());
      secondGPS = second(now());
#endif

  lcd.setCursor(0, 0); // top line *********
//  if (gps.time.isValid()) {
    sprintf(textBuffer, "%02d%c%02d%c%02d         UTC", hourGPS, HOUR_SEP, minuteGPS, MIN_SEP, secondGPS);
    lcd.print(textBuffer);
//  }

  // UTC date

  if (dayGPS != 0)
  {
    lcd.setCursor(0, 1); // line 1
    lcd.print(dayStr(weekdayGPS)); lcd.print("   "); // two more spaces 14.04.2018

    lcd.setCursor(10, 1);
    LcdDate(dayGPS, monthGPS, yearGPS); 
  }
  if (gps.location.isValid()) {

#ifndef DEBUG_MANUAL_POSITION
    latitude = gps.location.lat();
    lon = gps.location.lng();
#else
    latitude = latitude_manual;
    lon      = longitude_manual;
#endif

    char locator[7];
    Maidenhead(lon, latitude, locator);
    lcd.setCursor(0, 3); // last line *********
    lcd.print(locator); lcd.print("       ");
  }
  if (gps.satellites.isValid()) {
    noSats = gps.satellites.value();
    if (noSats < 10) lcd.setCursor(14, 3);
    else lcd.setCursor(13, 3); lcd.print(noSats); lcd.print(" Sats");
  }
}

// Menu item //////////////////////////////////////////////////////////////////////////////////////////
void LocalSun(           // local time, sun x 3
              int mode   // 0 for ScreenLocal
                         // 1 for ScreenLocalSunSimpler 
) {
  //
  // shows Actual (0 deg), Civil (-6 deg), and Nautical (-12 deg) sun rise/set
  //
  if (mode == 0)  LcdShortDayDateTimeLocal(0, 2);  // line 0
  else            LcdShortDayDateTimeLocal(0, 0);

  
  if (gps.location.isValid()) {
    if (minuteGPS != oldMinute) {
#ifndef DEBUG_MANUAL_POSITION
      latitude = gps.location.lat();
      lon = gps.location.lng();
#else
      latitude = latitude_manual;
      lon      = longitude_manual;
#endif

if (mode == 0)
{
      LcdSolarRiseSet(1, ' ',ScreenLocalSun);
      LcdSolarRiseSet(2, 'C',ScreenLocalSun);
      LcdSolarRiseSet(3, 'N',ScreenLocalSun);
}
else
{
      LcdSolarRiseSet(1, ' ',ScreenLocalSunSimpler);
      LcdSolarRiseSet(2, 'C',ScreenLocalSunSimpler);
      LcdSolarRiseSet(3, 'N',ScreenLocalSunSimpler);
}
    }
  }
  oldMinute = minuteGPS;
}


/*****
Purpose: Menu item
Finds local time, and three different sun rise/set times

Argument List: Global variables 

Return value: Displays on LCD
*****/

void LocalSunAzEl() { // local time, sun x 3
  //
  // shows Actual (0 deg), Civil (-6 deg), and Nautical (-12 deg) sun rise/set
  //
  LcdShortDayDateTimeLocal(0, 0);  // line 0
  if (gps.location.isValid()) {
    if (minuteGPS != oldMinute) {
    #ifndef DEBUG_MANUAL_POSITION
          latitude = gps.location.lat();
          lon = gps.location.lng();
    #else
          latitude = latitude_manual;
          lon      = longitude_manual;
    #endif

    LcdSolarRiseSet(1, ' ', ScreenLocalSunAzEl); // Actual Rise, Set times<
    LcdSolarRiseSet(2, 'O', ScreenLocalSunAzEl); //Noon info 
    LcdSolarRiseSet(3, 'Z', ScreenLocalSunAzEl); //Current Az El info

    }
  }
  oldMinute = minuteGPS;
}

/*****
Purpose: Menu item
Finds local time, Actual solar rise/set time and azimuth; lunar rise, set, illumination and azimuth

Argument List: Global variables 

Return value: Displays on LCD
*****/

void LocalSunMoon() { // local time, sun, moon
  //
  // shows solar rise/set in a chosen definition (Actual, Civil, ...)
  //

  LcdShortDayDateTimeLocal(0, 2);  // line 0, time offset 2 to the left

  if (gps.location.isValid()) {
    if (minuteGPS != oldMinute) {

#ifndef DEBUG_MANUAL_POSITION
      latitude = gps.location.lat();
      lon = gps.location.lng();
#else
      latitude = latitude_manual;
      lon      = longitude_manual;
#endif

      LcdSolarRiseSet(1, ' ', ScreenLocalSunMoon); // line 1, Actual rise time
//      LcdSolarRiseSet(2, 'C', 0); // line 2

      // MOON

      lcd.setCursor(0, 3);  // last line
      lcd.print("M ");

      // next rise / set
      short pRise, pSet, pTime;
      double rAz, sAz;
      int order;

      GetNextRiseSet(&pRise, &rAz, &pSet, &sAz, &order);
#ifdef FEATURE_SERIAL_MOON
      Serial.print(F("LocalSunMoon: order: ")); Serial.println(order);
#endif

      local = now() + utcOffset * 60;
      Hour = hour(local);
      Minute = minute(local);

      int packedTime = Hour * 100 + Minute;

      lcd.setCursor(2, 3); // last line

      // find next event
      if (order == 1)  // Rise
      {
        pTime = pRise;  lcd.write(UP_ARROW);
      }
      else // Set (or order not initialized correctly)
      {
        pTime = pSet; lcd.write(DOWN_ARROW);
      }

      if (pTime > -1)
      {
        int pHr  = pTime / 100;
        int pMin = pTime - 100 * pHr;
        PrintFixedWidth(lcd, pHr, 2); lcd.print(HOUR_SEP); PrintFixedWidth(lcd, pMin, 2, '0'); lcd.print(" ");
      }
      else lcd.print(" - ");

      float PhaseM, PercentPhaseM;
      MoonPhaseAccurate(PhaseM, PercentPhaseM);
      //MoonPhase(PhaseM, PercentPhaseM);

#ifdef FEATURE_SERIAL_MOON
      Serial.println(F("LocalSunMoon: "));
      Serial.print(F(" PhaseM, PercentPhaseM "));
      Serial.print(PhaseM); Serial.print(F(", ")); Serial.println(PercentPhaseM);
#endif

      lcd.setCursor(9, 3);
     
      MoonWaxWane(PhaseM); //arrow up/down or ' ' (space)
      MoonSymbol(PhaseM); // (, O, ), symbol

      PrintFixedWidth(lcd, (int)round(PercentPhaseM), 3);
      lcd.print("%");

      UpdateMoonPosition();
      lcd.setCursor(16, 3);
      PrintFixedWidth(lcd, (int)round(moon_elevation), 3);
      lcd.write(DEGREE);
    }
  }
  oldMinute = minuteGPS;
}

/*****
Purpose: Menu item
Finds local time, and lots of lunar info

Argument List: Global variables 

Return value: Displays on LCD
*****/

void LocalMoon() { // local time, moon phase, elevation, next rise/set

  String textbuf;
  float percentage;

  LcdShortDayDateTimeLocal(0, 1); // line 0, moved 1 position left to line up with next lines

  if (gps.location.isValid()) {
    if (minuteGPS != oldMinute) {  // update display every minute

      // days since last new moon
      float Phase, PercentPhase;

      UpdateMoonPosition();

      lcd.setCursor(0, 3); // line 3
      //
      //        lcd.print("  ");
      //        textbuf = String(moon_dist, 0);
      //        lcd.print(textbuf); lcd.print(" km");

      lcd.print(" ");
      PrintFixedWidth(lcd, int(round(moon_dist / 4067.0)), 3);
      lcd.print("% ");

      PrintFixedWidth(lcd, int(round(moon_dist / 1000.0)), 3);
      lcd.print("'km ");


      MoonPhase(Phase, PercentPhase);

      lcd.setCursor(14, 3);
      MoonWaxWane(Phase); // arrow
      MoonSymbol(Phase);  // (,0,)

      lcd.setCursor(16, 3);
      PrintFixedWidth(lcd, (int)(abs(round(PercentPhase))), 3);
      lcd.print("%");

#ifndef DEBUG_MANUAL_POSITION
      latitude = gps.location.lat();
      lon = gps.location.lng();
#else
      latitude = latitude_manual;
      lon      = longitude_manual;
#endif

      lcd.setCursor(0, 1); // line 1

      lcd.print("M El ");
      lcd.setCursor(4, 1);
      PrintFixedWidth(lcd, (int)round(moon_elevation), 4);
      lcd.write(DEGREE);

      lcd.setCursor(13, 1);
      lcd.print("Az ");
      PrintFixedWidth(lcd, (int)round(moon_azimuth), 3);
      lcd.write(DEGREE);

      // Moon rise or set time:
      short pRise, pSet, order;
      double rAz, sAz;
      float Az;
      int pTime, Symb;

      GetNextRiseSet(&pRise, &rAz, &pSet, &sAz, &order);
      /*
          pRise = pSet = -2;  // the moon never sets
          pRise = pSet = -1;  // the moon never rises
          pRise = -1;               // no MoonRise and the moon sets
          pSet = -1;                // the moon rises and never sets
      */
      local = now() + utcOffset * 60;
      Hour = hour(local);
      Minute = minute(local);

      int packedTime = Hour * 100 + Minute;

      // find next event
      if (order == 1)
        // Rise
      {
        pTime = pRise;  Symb = UP_ARROW; Az = rAz;
      }
      else
      {
        pTime = pSet; Symb = DOWN_ARROW; Az = sAz;
      }

      lcd.setCursor(2, 2); // line 2

      if (pTime > -1)
      {
        int pHr  = pTime / 100;
        int pMin = pTime - 100 * pHr;

        lcd.write(Symb); lcd.print("   ");
        PrintFixedWidth(lcd, pHr, 2); lcd.print(HOUR_SEP); PrintFixedWidth(lcd, pMin, 2, '0'); lcd.print(" ");
        lcd.setCursor(13, 2);
        lcd.print("Az ");
        PrintFixedWidth(lcd, (int)round(Az), 3);
        lcd.write(DEGREE);
      }
      else lcd.print("  No Rise/Set       ");

      oldMinute = minuteGPS;
    }
  }
}

/*****
Purpose: Menu item
Finds lunar rise/set times for the next couple of days

Argument List: Global variables 

Return value: Displays on LCD

Issues: follows UTC day/night - may get incorrect sorting of rise/set times if timezone is very different from UTC?
*****/

void MoonRiseSet(void) {

  if (gps.location.isValid()) {

#ifndef DEBUG_MANUAL_POSITION
    latitude = gps.location.lat();
    lon = gps.location.lng();
#else
    latitude = latitude_manual;
    lon      = longitude_manual;
#endif

    if (minuteGPS != oldMinute) {

      short pRise, pSet, pRise2, pSet2, packedTime; // time in compact format '100*hr + min'
      double rAz, sAz, rAz2, sAz2;

      local = now() + utcOffset * 60;
      Hour = hour(local);
      Minute = minute(local);

      packedTime = Hour * 100 + Minute; // local time 19.11.2021

      // ***** rise/set for this UTC day:
      
      GetMoonRiseSetTimes(float(utcOffset) / 60.0, latitude, lon, &pRise, &rAz, &pSet, &sAz);

      lcd.setCursor(0, 0); // top line
      lcd.print("M ");

      int lineNo = 0;
      int lineUsed = 0;

      int MoonRiseHr  = pRise / 100;
      int MoonRiseMin = pRise - 100 * MoonRiseHr;
      int MoonSetHr  = pSet / 100;
      int MoonSetMin = pSet - 100 * MoonSetHr;   

      lcd.setCursor(2, lineNo);     // row no 0

 // Determine if there is two, one or no rise/set events on the present date and which are in the future

      int NoOfEvents = 0;
      if (packedTime < pRise | packedTime < pSet) NoOfEvents = 1;
      if (packedTime < pRise & packedTime < pSet) NoOfEvents = 2;

#ifdef FEATURE_SERIAL_MOON
  Serial.println(NoOfEvents);
#endif

      if (NoOfEvents == 2)
      {
          if (pRise < pSet) lcd.setCursor(2, lineNo);     // row no 0
          else              lcd.setCursor(2, lineNo + 1); // row no 1  
      }
      
      if (pRise > -1 & pRise > packedTime) // only show a future event
      {        
        lcd.write(UP_ARROW); lcd.print(" ");
        PrintFixedWidth(lcd, MoonRiseHr, 2, '0');   lcd.print(HOUR_SEP);
        PrintFixedWidth(lcd, MoonRiseMin, 2, '0');  lcd.print("  ");
        PrintFixedWidth(lcd, (int)round(rAz), 4);
        lcd.write(DEGREE); lcd.print("  ");
        lineUsed = lineUsed + 1;
      }
//      else if (pRise < 0)
//      {
//        lcd.print(pRise); lcd.print("              ");
//      }
#ifdef FEATURE_SERIAL_MOON
  Serial.println(lineUsed);
#endif

      if (NoOfEvents == 2)
      {
        if (pRise < pSet) lcd.setCursor(2, lineNo + 1); // row no 1  
        else              lcd.setCursor(2, lineNo);     // row no 0
      }
      
      if (pSet > -1 & pSet > packedTime) // only show a future event
      {      
        lcd.write(DOWN_ARROW);  lcd.print(" ");
        PrintFixedWidth(lcd, MoonSetHr, 2, '0');   lcd.print(HOUR_SEP); // doesn't handle 00:48 well with ' ' as separator
        PrintFixedWidth(lcd, MoonSetMin, 2, '0');  lcd.print("  ");
        PrintFixedWidth(lcd, (int)round(sAz), 4); lcd.write(DEGREE); lcd.print("  ");
        lineUsed = lineUsed + 1;
      }
//      else if (pSet < 0)
//      {
//        lcd.print(pSet); lcd.print("              ");
//      }

#ifdef FEATURE_SERIAL_MOON
  Serial.println(lineUsed);
#endif

      lineNo = lineUsed;

      // ****** rise/set for next UTC day:
      
      GetMoonRiseSetTimes(float(utcOffset) / 60.0 - 24.0, latitude, lon, &pRise2, &rAz2, &pSet2, &sAz2);

      // Rise and set times for moon:

      MoonRiseHr  = pRise2 / 100;
      MoonRiseMin = pRise2 - 100 * MoonRiseHr;
      MoonSetHr  = pSet2 / 100;
      MoonSetMin = pSet2 - 100 * MoonSetHr;

      if (pRise2 < pSet2) lcd.setCursor(2, lineNo);
      else              lcd.setCursor(2, lineNo + 1);

      lcd.write(UP_ARROW); lcd.print(" ");

      if (pRise2 > -1) {
        PrintFixedWidth(lcd, MoonRiseHr, 2, '0');   lcd.print(HOUR_SEP);
        PrintFixedWidth(lcd, MoonRiseMin, 2, '0');  lcd.print("  ");
        PrintFixedWidth(lcd, (int)round(rAz2), 4);
        lcd.write(DEGREE); lcd.print("  ");
         lineUsed = lineUsed + 1;
      }
//      else
//      {
//        lcd.print(pRise2); lcd.print("              ");
//      }

      if (pRise2 < pSet2) lcd.setCursor(2, lineNo + 1);
      else              lcd.setCursor(2, lineNo);

      lcd.write(DOWN_ARROW);  lcd.print(" ");
      if (pSet2 > -1) {
        PrintFixedWidth(lcd, MoonSetHr, 2, '0');   lcd.print(HOUR_SEP); // doesn't handle 00:48 well with ' ' as separator
        PrintFixedWidth(lcd, MoonSetMin, 2, '0');   lcd.print("  ");
        PrintFixedWidth(lcd, (int)round(sAz2), 4); lcd.write(DEGREE); lcd.print("  ");
         lineUsed = lineUsed + 1;
      }
//      else
//      {
//        lcd.print(pSet2); lcd.print("              ");
//      }

    
      lineNo = lineUsed;

      if (lineNo <= 3)
      // **** if there is room add a line or two more
      // rise/set for next UTC day:
      {     
        GetMoonRiseSetTimes(float(utcOffset) / 60.0 - 48.0, latitude, lon, &pRise2, &rAz2, &pSet2, &sAz2);
  
        // Rise and set times for moon:
  
        MoonRiseHr  = pRise2 / 100;
        MoonRiseMin = pRise2 - 100 * MoonRiseHr;
        MoonSetHr  = pSet2 / 100;
        MoonSetMin = pSet2 - 100 * MoonSetHr;
  
        if (pRise2 < pSet2) 
        {
          lcd.setCursor(2, lineNo);
          lineUsed = lineNo;
        }
        else
        {
          lcd.setCursor(2, lineNo + 1);
          lineUsed = lineNo + 1;
        }

        if (lineUsed <= 3)
        {
          lcd.write(UP_ARROW); lcd.print(" ");
    
          if (pRise2 > -1) {
            PrintFixedWidth(lcd, MoonRiseHr, 2, '0');   lcd.print(HOUR_SEP);
            PrintFixedWidth(lcd, MoonRiseMin, 2, '0');  lcd.print("  ");
            PrintFixedWidth(lcd, (int)round(rAz2), 4);
            lcd.write(DEGREE); lcd.print("  ");
          }
          else
          {
            lcd.print(pRise2); lcd.print("              ");
          }
        }
  
        if (pRise2 < pSet2) 
        {
          lcd.setCursor(2, lineNo + 1);
          lineUsed = lineNo + 1;
        }
        else
        {
          lcd.setCursor(2, lineNo);
          lineUsed = lineNo;
        }

        if (lineUsed <=3)
        {
          lcd.write(DOWN_ARROW);  lcd.print(" ");
          if (pSet2 > -1) {
            PrintFixedWidth(lcd, MoonSetHr, 2, '0');   lcd.print(HOUR_SEP); // doesn't handle 00:48 well with ' ' as separator
            PrintFixedWidth(lcd, MoonSetMin, 2, '0');   lcd.print("  ");
            PrintFixedWidth(lcd, (int)round(sAz2), 4); lcd.write(DEGREE); lcd.print("  ");
          }
          else
          {
            lcd.print(pSet2); lcd.print("              ");
          }
        }

      }
      
      lcd.setCursor(18, 3); lcd.print("  ");
    }
  }
  oldMinute = minuteGPS;
}

/*****
Purpose: Menu item
Finds local time in binary formats hour, minute, second

Argument List: int mode = 0 - vertical BCD
                   mode = 1 - horizontal BCD
                   mode = 2 - horisontal binary

Return value: Displays on LCD
*****/

void Binary(int mode) { // binary local time

  char textBuffer[12]; // was [9] -> memory overwrite
  int tens, ones;

  int BinaryTensHour[6], BinaryHour[6], BinaryTensMinute[6], BinaryMinute[6], BinaryTensSeconds[6], BinarySeconds[6];

  // get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  // convert to BCD

  // must send a variable, not an equation, to DecToBinary as it does in-place arithmetic on input variable
  ones = Hour % 10; tens = (Hour - ones) / 10;
  DecToBinary(ones, BinaryHour);
  DecToBinary(tens, BinaryTensHour);

  ones = Minute % 10; tens = (Minute - ones) / 10;
  DecToBinary(tens, BinaryTensMinute); DecToBinary(ones, BinaryMinute);

  ones = Seconds % 10;   tens = (Seconds - ones) / 10;
  DecToBinary(tens, BinaryTensSeconds); DecToBinary(ones, BinarySeconds);


  if (mode == 0) // vertical digits:
  {
    lcd.setCursor(0, 0); lcd.print("BCD");

    lcd.setCursor(9, 0);
    sprintf(textBuffer, " %1d  %1d  %1d", BinaryHour[2], BinaryMinute[2], BinarySeconds[2]);
    lcd.print(textBuffer);
    lcd.setCursor(19, 0); lcd.print("8");

    //   lcd.setCursor(0,1); lcd.print("hh mm ss");
    lcd.setCursor(9, 1);
    sprintf(textBuffer, " %1d %1d%1d %1d%1d", BinaryHour[3], BinaryTensMinute[3], BinaryMinute[3], BinaryTensSeconds[3], BinarySeconds[3]);
    lcd.print(textBuffer);
    lcd.setCursor(19, 1); lcd.print("4");

    lcd.setCursor(9, 2);
    sprintf(textBuffer, "%1d%1d %1d%1d %1d%1d", BinaryTensHour[4], BinaryHour[4], BinaryTensMinute[4], BinaryMinute[4], BinaryTensSeconds[4], BinarySeconds[4]);
    lcd.print(textBuffer);
    lcd.setCursor(19, 2); lcd.print("2");


    lcd.setCursor(9, 3); //LSB
    sprintf(textBuffer, "%1d%1d %1d%1d %1d%1d  ", BinaryTensHour[5], BinaryHour[5], BinaryTensMinute[5], BinaryMinute[5], BinaryTensSeconds[5], BinarySeconds[5]);
    lcd.print(textBuffer);
    lcd.setCursor(19, 3); lcd.print("1");
  }
  else if (mode == 1)
  {
    //// horizontal BCD digits:

    lcd.setCursor(0, 0); lcd.print("BCD");

    lcd.setCursor(9, 1); sprintf(textBuffer, "  %1d%1d ", BinaryTensHour[4], BinaryTensHour[5] );
    lcd.print(textBuffer);
    sprintf(textBuffer, "%1d%1d%1d%1d H", BinaryHour[2], BinaryHour[3], BinaryHour[4], BinaryHour[5]);
    lcd.print(textBuffer);

    lcd.setCursor(9, 2);  sprintf(textBuffer, " %1d%1d%1d ", BinaryTensMinute[3], BinaryTensMinute[4], BinaryTensMinute[5] );
    lcd.print(textBuffer);
    sprintf(textBuffer, "%1d%1d%1d%1d M", BinaryMinute[2], BinaryMinute[3], BinaryMinute[4], BinaryMinute[5] );
    lcd.print(textBuffer);

    lcd.setCursor(9, 3);  sprintf(textBuffer, " %1d%1d%1d ", BinaryTensSeconds[3], BinaryTensSeconds[4], BinaryTensSeconds[5] );
    lcd.print(textBuffer);
    sprintf(textBuffer, "%1d%1d%1d%1d S", BinarySeconds[2], BinarySeconds[3], BinarySeconds[4], BinarySeconds[5] );
    lcd.print(textBuffer);




    if (Seconds < SECONDS_CLOCK_HELP)  // show help: weighting
    {
      lcd.setCursor(9, 0); lcd.print(" 421 8421");
    }
    else
    {
      lcd.setCursor(9, 0); lcd.print("         ");
    }
  }
  else
    // horisontal 5/6-bit binary
  {
    // convert to binary:
    DecToBinary(Hour, BinaryHour);
    DecToBinary(Minute, BinaryMinute);
    DecToBinary(Seconds, BinarySeconds);

    lcd.setCursor(13, 1); sprintf(textBuffer, "%1d%1d%1d%1d%1d H", BinaryHour[1], BinaryHour[2], BinaryHour[3], BinaryHour[4], BinaryHour[5]);
    lcd.print(textBuffer);

    lcd.setCursor(12, 2); sprintf(textBuffer, "%1d%1d%1d%1d%1d%1d M", BinaryMinute[0], BinaryMinute[1], BinaryMinute[2], BinaryMinute[3], BinaryMinute[4], BinaryMinute[5]);
    lcd.print(textBuffer);

    lcd.setCursor(12, 3); sprintf(textBuffer, "%1d%1d%1d%1d%1d%1d S", BinarySeconds[0], BinarySeconds[1], BinarySeconds[2], BinarySeconds[3], BinarySeconds[4], BinarySeconds[5] );
    lcd.print(textBuffer);

    lcd.setCursor(0, 0); lcd.print("Binary");

    if (Seconds < SECONDS_CLOCK_HELP)  // show help: weighting
    {
      lcd.setCursor(13, 0); lcd.print(" 8421");
    }
    else
    {
      lcd.setCursor(13, 0); lcd.print("     ");
    }
  }

  // Common for all modes:

  if (Seconds < SECONDS_CLOCK_HELP)  // show time in normal numbers
  {
    sprintf(textBuffer, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
  }
  else
  {
    sprintf(textBuffer, "        ");
  }
  lcd.setCursor(0, 3); // last line *********
  lcd.print(textBuffer);
}

/*****
Purpose: Menu item
Finds local time as a bar display

Argument List: None

Return value: Displays on LCD
*****/

void Bar(void) {
  char textBuffer[9];
  // get local time

  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  // use a 12 character bar  just like a 12 hour clock with ticks every hour
  // for second ticks use ' " % #
  lcd.setCursor(0, 0);
  int imax = Hour;

  if (Hour == 13 & Minute == 0 & Seconds == 0)
  {
    lcd.print("                   ");
    lcd.setCursor(0, 0);
  }

  if (Hour > 12) imax = Hour - 12;

  if (Hour == 0) lcd.print("                ");
  for (int i = 0; i < imax; i++) {
    lcd.write(ALL_ON); // fills square
    if (i == 2 | i == 5 | i == 8) lcd.write(ALL_OFF); // empty
  }

  // could have used |, ||, |||, |||| for intermediate symbols by creating new characters
  // like here https://forum.arduino.cc/index.php?topic=180678.0
  // but easier to use something standard

  //  if (Minute/12 == 1) {lcd.write(BIG_DOT);}
  //  else if (Minute/12 == 2) {lcd.print('"');}
  //  else if (Minute/12 == 3) {lcd.write(THREE_LINES);}
  //  else if (Minute/12 == 4) {lcd.write(SQUARE);}

  //  lcd.print(" ");lcd.print(Hour);

  lcd.setCursor(18, 0); lcd.print("1h");

  lcd.setCursor(0, 1);
  imax = Minute / 5;
  if (Minute == 0) lcd.print("                ");
  for (int i = 0; i < imax; i++) {
    lcd.write(ALL_ON); // fills square
    if (i == 2 | i == 5 | i == 8) lcd.write(ALL_OFF); // empty
  }
  if (Minute % 5 == 1) {
    lcd.write(BIG_DOT);
  }
  else if (Minute % 5 == 2) {
    lcd.print('"');
  }
  else if (Minute % 5 == 3) {
    lcd.write(THREE_LINES);
  }
  else if (Minute % 5 == 4) {
    lcd.write(SQUARE);
  }
  // lcd.print(" ");lcd.print(Minute);
  lcd.setCursor(18, 1); lcd.print("5m");


  // seconds in 12 characters, with a break every 3 characters
  lcd.setCursor(0, 2);
  imax = Seconds / 5;
  if (Seconds == 0) lcd.print("                ");
  for (int i = 0; i < imax; i++) {
    lcd.write(ALL_ON); // fills square
    if (i == 2 | i == 5 | i == 8) lcd.write(ALL_OFF); // empty
  }
  if (Seconds % 5 == 1) {
    lcd.write(BIG_DOT);
  }
  else if (Seconds % 5 == 2) {
    lcd.print('"');
  }
  else if (Seconds % 5 == 3) {
    lcd.write(THREE_LINES);
  }  //("%");}
  else if (Seconds % 5 == 4) {
    lcd.write(SQUARE);
  }  //("#");}
  lcd.setCursor(18, 2); lcd.print("5s");
  //        lcd.setCursor(18, 3); lcd.print("  ");

  lcd.setCursor(18, 3);
  if (Hour > 12) lcd.print("PM");
  else lcd.print("AM");

  lcd.setCursor(8, 3);
  if (Seconds < SECONDS_CLOCK_HELP)  // show time in normal numbers
  {
    //    lcd.print("Bar");
    sprintf(textBuffer, "%02d%c%02d%c%02d", Hour % 12, HOUR_SEP, Minute, MIN_SEP, Seconds);

    lcd.print(textBuffer);

  }
  else
  {
    lcd.print("          ");
  }
}

/*****
Purpose: Menu item
Finds local time as Set theory clock of https://en.wikipedia.org/wiki/Mengenlehreuhr in Berlin

Argument List: None

Return value: Displays on LCD
*****/

void MengenLehrUhr(void) {
  //
  
  int imax;
  //  lcd.clear(); // makes it blink

  // get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  lcd.setCursor(0, 0);
  // top line has 5 hour resolution
  if (Hour > 4)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");
  if (Hour > 9)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  if (Hour > 14)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");
  if (Hour > 19)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  lcd.setCursor(18, 0); lcd.print("5h");

  // second line shows remainder and has 1 hour resolution

  lcd.setCursor(0, 1);
  imax = Hour % 5;
  if (imax > 0)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");
  if (imax > 1)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");
  if (imax > 2)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");
  if (imax > 3)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  lcd.setCursor(18, 1); lcd.print("1h");

  // third line shows a bar for every 5 minutes
  int ii;
  lcd.setCursor(0, 2);

  // only overwrite old characters when needed, to avoid flicker

  imax = Minute / 5;
  if (imax == 0)
  {
    lcd.print("                  ");
    lcd.setCursor(0, 2);
  }

  for (ii = 0; ii < imax; ii++)
  {
    lcd.write(ALL_ON);
    if (ii == 2 || ii == 5 || ii == 8) lcd.print(" ");
  }

  lcd.setCursor(18, 2); lcd.print("5m");

  // fourth line shows remainder and has 1 minute resolution
  lcd.setCursor(0, 3);

  imax = Minute % 5;
  if (imax > 0)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  if (imax > 1)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  if (imax > 2)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  if (imax > 3)
  {
    lcd.print("-"); lcd.write(ALL_ON); lcd.print("-"); lcd.print(" ");// fills square
  }
  else lcd.print("    ");

  lcd.setCursor(18, 3); lcd.print("1m");
}

/*****
Purpose: Menu item
Finds local time as Linear Clock, https://de.wikipedia.org/wiki/Linear-Uhr in Kassel

Argument List: None

Return value: Displays on LCD
*****/

void LinearUhr(void) {
  

  int imax;
  int ii;

  // get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  lcd.setCursor(0, 0);
  // top line has 10 hour resolution
  imax = Hour / 10;
  lcd.print("             ");
  lcd.setCursor(0, 0);
  for (ii = 0; ii < imax; ii++)
  {
    lcd.write(ALL_ON);
  }
  lcd.setCursor(17, 0); lcd.print("10h");

  // second line shows remainder and has 1 hour resolution
  lcd.setCursor(0, 1);
  imax = Hour % 10;

  // only overwrite old characters when needed, to avoid flicker
  if (imax == 0)
  {
    lcd.print("             ");
    lcd.setCursor(0, 1);
  }
  for (ii = 0; ii < imax; ii++)
  {
    lcd.write(ALL_ON);
    if (ii == 4) lcd.print(" ");
  }
  lcd.setCursor(18, 1); lcd.print("1h");

  // third line shows a bar for every 10 minutes
  lcd.setCursor(0, 2);
  imax = Minute / 10;

  // only overwrite old characters when needed, to avoid flicker
  if (imax == 0)
  {
    lcd.print("                  ");
    lcd.setCursor(0, 2);
  }

  for (ii = 0; ii < imax; ii++)
  {
    lcd.write(ALL_ON);
    if (ii == 4) lcd.print(" ");
  }
  lcd.setCursor(17, 2); lcd.print("10m");

  // last line shows remainder and has 1 minute resolution
  lcd.setCursor(0, 3);

  // only overwrite old characters when needed, to avoid flicker

  imax = Minute % 10;
  if (imax == 0)
  {
    lcd.print("                  ");
    lcd.setCursor(0, 3);
  }

  for (ii = 0; ii < imax; ii++)
  {
    lcd.write(ALL_ON);
    if (ii == 4) lcd.print(" ");
  }
  lcd.setCursor(18, 3); lcd.print("1m");
}

/*****
Purpose: Menu item
Finds internal time: jd, j2000, etc

Argument List: None

Return value: Displays on LCD
*****/

void InternalTime() {     // UTC, Unix time, J2000, etc
  char textBuffer[20];

  lcd.setCursor(0, 0); // top line *********
  if (gps.time.isValid()) {

    float jd = now() / 86400.0; // cdn(now()); // now/86400, i.e. no of days since 1970
    float j2000 = jd - 10957.5; // 1- line
    lcd.print("j2k ");
    lcd.print(j2000);

    #ifdef FEATURE_PC_SERIAL_GPS_IN
        hourGPS = hour(now());
        minuteGPS = minute(now());
        secondGPS = second(now());
    #endif

    lcd.setCursor(12, 0);
    sprintf(textBuffer, "%02d%c%02d%c%02d UTC ", hourGPS, HOUR_SEP, minuteGPS, MIN_SEP, secondGPS);
    lcd.print(textBuffer);

    lcd.setCursor(0, 1);
    lcd.print("jd1970 ");
    lcd.print(jd);

    // utc = now(); // UNIX time, seconds ref to 1970
    lcd.setCursor(0, 2);
    lcd.print("now    ");
    lcd.print(now());

    lcd.setCursor(0, 3);
    lcd.print("local  ");
    local = now() + utcOffset * 60;
    lcd.print(local);
  }
}
/*****
Purpose: Menu item
Gives code status

Argument List: None

Return value: Displays on LCD
*****/

void CodeStatus(void) {
  lcd.setCursor(0, 0); lcd.print("* LA3ZA GPS clock *");
  lcd.setCursor(0, 1); lcd.print(codeVersion);
  lcd.setCursor(0, 2); lcd.print("GPS  "); lcd.print(gpsBaud); //lcd.print(" bps");
  lcd.setCursor(0, 3); lcd.print(tcr -> abbrev); lcd.setCursor(5, 3); PrintFixedWidth(lcd, utcOffset, 4); //timezone name and offset (min)
}

/*****
Purpose: Menu item
Gives UTC time, locator, latitude/longitude, altitude and no of satellites

Argument List: None

Return value: Displays on LCD
*****/

void UTCPosition() {     // position, altitude, locator, # satellites
  String textbuf;

  LcdUTCTimeLocator(0); // top line *********
  // UTC date
  if (gps.location.isValid()) {

#ifndef DEBUG_MANUAL_POSITION
    latitude = gps.location.lat();
    lon = gps.location.lng();
    alt = gps.altitude.meters();
#else
    latitude = latitude_manual;
    lon      = longitude_manual;
    alt = 0.0;
#endif



    int cycleTime = 10; // 4.10.2022: was 4 seconds

    lcd.setCursor(0, 2);
    if ((now() / cycleTime) % 3 == 0) { // change every cycleTime seconds

      //  decimal degrees
      lcd.setCursor(0, 2);
      textbuf = String(abs(latitude), 4);
      lcd.print(textbuf); lcd.write(DEGREE);
      if (latitude < 0) lcd.print(" S   ");
      else lcd.print(" N   ");

      lcd.setCursor(0, 3);
      textbuf = String(abs(lon), 4);
      int strLength = textbuf.length();
      lcd.print(textbuf); lcd.write(DEGREE);
      if (lon < 0) lcd.print(" W    ");
      else lcd.print(" E    ");
    }
    else if ((now() / cycleTime) % 3 == 1) {

      // degrees, minutes, seconds
      lcd.setCursor(0, 2);
      float mins;
      textbuf = String((int)abs(latitude));
      lcd.print(textbuf); lcd.write(DEGREE);
      mins = abs(60 * (latitude - (int)latitude));  // minutes
      textbuf = String((int)mins);
      lcd.print(textbuf); lcd.write("'");
      textbuf = String((int)(abs(60 * (mins - (int)mins)))); // seconds
      lcd.print(textbuf); lcd.write(34);
      if (latitude < 0) lcd.print(" S  ");
      else lcd.print(" N  ");


      lcd.setCursor(0, 3);
      textbuf = String((int)abs(lon));
      lcd.print(textbuf);
      lcd.write(DEGREE);
      mins = abs(60 * (lon - (int)lon));
      textbuf = String((int)mins);
      lcd.print(textbuf); lcd.write("'");
      textbuf = String((int)(abs(60 * (mins - (int)(mins)))));
      lcd.print(textbuf); lcd.write(34); // symbol for "
      if (lon < 0) lcd.print(" W ");
      else lcd.print(" E ");
    }

    else  {

      // degrees, decimal minutes
      lcd.setCursor(0, 2);
      float mins;
      textbuf = String(int(abs(latitude)));
      lcd.print(textbuf); lcd.write(DEGREE);
      mins = abs(60 * (latitude - (int)latitude));
      textbuf = String(abs(mins), 2);
      lcd.print(textbuf);
      if (latitude < 0) lcd.print("' S ");
      else lcd.print("' N ");

      lcd.setCursor(0, 3);
      textbuf = String(int(abs(lon)));
      lcd.print(textbuf);
      lcd.write(DEGREE);
      mins = abs(60 * (lon - (int)lon));
      textbuf = String(abs(mins), 2);  // double abs() to avoid negative number for x.00 degrees
      lcd.print(textbuf);
      if (lon < 0) lcd.print("' W  ");
      else lcd.print("' E  ");
    }
  }
  // enough space on display for 2469 m
  lcd.setCursor(14, 2);
  PrintFixedWidth(lcd, (int)round(alt), 4, ' '); lcd.print(" m");

  if (gps.satellites.isValid()) {
    noSats = gps.satellites.value();
    if (noSats < 10) lcd.setCursor(14, 3);
    else lcd.setCursor(13, 3); lcd.print(noSats); lcd.print(" Sats");
  }
}


/*****
Purpose: Menu item
Finds data for beacons of NCDXF  Northern California DX Foundation

Argument List: int option - option=1: 14-21 MHz beacons on lines 1-3,  option=2: 21-28 MHz beacons on lines 1-3

Return value: Displays on LCD
*****/

void NCDXFBeacons(
  int option // option=1: 14-21 MHz beacons on lines 1-3,  option=2: 21-28 MHz beacons on lines 1-3
) {     // UTC + info about NCDXF beacons

  // Inspired by OE3GOD: https://www.hamspirit.de/7757/eine-stationsuhr-mit-ncdxf-bakenanzeige/

  int ii, iii, iiii, km, bandOffset;
  double lati, longi;
  char* callsign[19] = {
    " 4U1UN", " VE8AT", "  W6WX", " KH6RS", "  ZL6B", "VK6RBP", "JA2IGY", "  RR9O", "  VR2B", "  4S7B", " ZS6DN",
    "  5Z4B", " 4X6TU", "  OH2B", "  CS3B", " LU4AA", "  OA4B", "  YV5B"
  };
  char* location[19] = {
    "FN30as", "EQ79ax", "CM97bd", "BL10ts", "RE78tw", "OF87av", "PM84jk", "NO14kx", "OL72bg", "MJ96wv", "KG44dc",
    "KI88ks", "KM72jb", "KP20dh", "IM12or", "GF05tj", "FH17mw", "FJ69cc"
  };
  // OH2B @ KP20dh and not just KP20: https://automatic.sral.fi/?stype=beacon&language=en
  char* qth[19] = {
    "N York ", "Nunavut", "Califor", "Hawaii ", "N Zeala", "Austral", "Japan  ", "Siberia", "H Kong ", "Sri Lan", "S Afric",
    "Kenya  ", "Israel ", "Finland", "Madeira", "Argenti", "Peru   ", "Venezue"
  };
  char* qrg[6] = {"14100", "18110", "21150", "24930", "28200"};

  LcdUTCTimeLocator(0); // top line *********
  /*
     Each beacon transmits once on each band once every three minutes, 24 hours a day.
     At the end of each 10 second transmission, the beacon steps to the next higher band
     and the next beacon in the sequence begins transmitting.
  */
  ii = (60 * (minuteGPS % 3) + secondGPS) / 10; // ii from 0 to 17

  if (option <= 1) bandOffset = 0; // 14-18 MHz
  else             bandOffset = 2; // 18-28 MHz

  for (iiii = 1; iiii < 4; iiii += 1) { // step over lines 1,2,3
    lcd.setCursor(0, iiii);
    //
    // modulo for negative numbers: https://twitter.com/parkerboundy/status/326924215833985024
    iii = ((ii - iiii + 1 - bandOffset % 18) + 18) % 18;
    lcd.print(qrg[iiii - 1 + bandOffset]); lcd.print(" "); lcd.print(callsign[iii]);

    if (secondGPS % 10 < 5) {
      lcd.print(" ");  // first half of cycle: location
      lcd.print(qth[iii]);
    }
    else              // second half of cycle: distance
    {
      LocatorToLatLong(location[iii], lati, longi);// position of beacon
      km = Distance(lati, longi, latitude, lon);    // distance beacon - GPS
      PrintFixedWidth(lcd, (int)float(km), 6);
      lcd.print("km");
    }
  }
}

/*****
Purpose: Menu item
Shows data for WSPR (Weak Signal Propagation Reporter) coordinated mode, frequency

Argument List: none

Return value: Displays on LCD
*****/

void WSPRsequence() {     // UTC, + WSPR band/frequency for coordinated WSPR
  // https://physics.princeton.edu//pulsar/K1JT/doc/wspr/wspr-main.html#BANDHOPPING
  // 20 min cycle over 10 bands from 160m to 10m

  int ii;
  char* band[11] = {"160", "80", "60", "40", "30", "20", "17", "15", "12", "10"};
  char* qrg[11] = {"1838.100", "3570.100", "5366.200", "7040.100", "10140.200", "14097.100", "18106.100", "21096.100", "24926.100", "28126.100"};

  LcdUTCTimeLocator(0); // top line *********
  /*
     Each WSPR frequency is transmitted every 20 minutes in 2 min intervals.
  */
  lcd.setCursor(0, 2);
  lcd.print("WSPR band hopping:  ");

  ii = (minuteGPS % 20) / 2; // ii from 0 to 9

  // WSPR transmission starts 1 second into even minute and lasts for 110.6 = 60 + 50.6 seconds
  if ((minuteGPS % 2 == 0 && secondGPS < 1) || (minuteGPS % 2 == 1 && secondGPS > 52))
  {
    lcd.setCursor(0, 3); lcd.print("                    ");
  }
  else
  {
    lcd.setCursor(0, 3); lcd.print(band[ii]); lcd.print(" m "); lcd.print(qrg[ii]);  lcd.print(" kHz  ");
  }
  lcd.setCursor(19, 3); lcd.print(" "); // blank out menu number
}

/*****
Purpose: Menu item
Shows clock in hex, octal, or binary format

Argument List: int val = 0 - hex, 1 - octal, 3 - hex, octal, and binary

Return value: Displays on LCD
*****/

void HexOctalClock(int val)   
{
  char textbuf[21];
  int  BinaryHour[6],  BinaryMinute[6],  BinarySeconds[6];

  //  get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  lcd.setCursor(0, 0);
  if (val == 0)      lcd.print("Hex   ");
  else if (val == 1) lcd.print("Oct   ");

  if (val == 0)     // Hex
  {
    lcd.setCursor(7, 1);
    sprintf(textbuf, "%02X%c%02X%c%02X", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
    lcd.print(textbuf);
    lcd.setCursor(17,3); lcd.print("   ");
  }
  else if (val == 1) // Oct
  {
    lcd.setCursor(7, 1);
    sprintf(textbuf, "%02o%c%02o%c%02o", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
    lcd.print(textbuf);
    lcd.setCursor(17,3); lcd.print("   ");
  }
  else // Binary, oct, dec, hex
  {
    // convert to binary:
    DecToBinary(Hour, BinaryHour);
    DecToBinary(Minute, BinaryMinute);
    DecToBinary(Seconds, BinarySeconds);

    lcd.setCursor(0, 0); sprintf(textbuf, "%1d%1d%1d%1d%1d", BinaryHour[1], BinaryHour[2], BinaryHour[3], BinaryHour[4], BinaryHour[5]);
    lcd.print(textbuf); lcd.print(HOUR_SEP);

    sprintf(textbuf, "%1d%1d%1d%1d%1d%1d", BinaryMinute[0], BinaryMinute[1], BinaryMinute[2], BinaryMinute[3], BinaryMinute[4], BinaryMinute[5] );
    lcd.print(textbuf); lcd.print(MIN_SEP);

    sprintf(textbuf, "%1d%1d%1d%1d%1d%1dB", BinarySeconds[0], BinarySeconds[1], BinarySeconds[2], BinarySeconds[3], BinarySeconds[4], BinarySeconds[5] );
    lcd.print(textbuf);
    
    lcd.setCursor(19, 1); lcd.print("O");
    lcd.setCursor(7, 1);
    sprintf(textbuf, "%02o%c%02o%c%02o", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds); // octal
    lcd.print(textbuf);
    
    lcd.setCursor(18, 3); lcd.print(" H");
    lcd.setCursor(7, 3);
    sprintf(textbuf, "%02X%c%02X%c%02X", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds); // hex
    lcd.print(textbuf);
  }


  //  help screen with normal clock for 0...SECONDS_CLOCK_HELP seconds per minute
  if (Seconds < SECONDS_CLOCK_HELP | val==3)  // show time in normal numbers - always if simultaneous Bin, Oct, Hex
  {
    if (val == 3) {
      lcd.setCursor(19, 2); lcd.print("D");
    }
    else
    {
      lcd.setCursor(18, 3); lcd.print("  "); // clear number in lower left corner
    }
    lcd.setCursor(7, 2);
    sprintf(textbuf, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
    lcd.print(textbuf);
  }
  else
  {
    lcd.setCursor(0, 2);
    lcd.print("                    ");
  }

}


/*****
Purpose: Menu item
Shows date for Easter for the next years after  Gregorian (Western) aad Julian (Eastern) calendars
Output dates are in Gregorian dates

Argument List: int yr -  input value for year

Return value: Displays on LCD
*****/
void EasterDates(int yr)
{
  /* Parameters K, E:
      Julian calendar:
      K=-3 E=-1

      Gregorian calendar:
      1583-1693 K= l E=-8
      1700-1799 K= 0 E=-9
      1800-1899 K=-l E=-9
      1900-2099 K=-2 E=-10
      2100-2199 K=-3 E=-10
  */

  int K, E;
  int ii = 1;
  int PaschalFullMoon, EasterDate, EasterMonth;
  lcd.setCursor(0, 0); lcd.print("Easter  Greg. Julian");

if (minuteGPS != oldMinute) {

  for (int yer = yr; yer < yr + 3; yer++)
  {
    lcd.setCursor(2, ii); lcd.print(yer); lcd.print(": ");
    // Gregorian (West):
    K = -2; E = -10;
    ComputeEasterDate(yer, K, E, &PaschalFullMoon, &EasterDate, &EasterMonth);
    lcd.setCursor(8, ii); LcdDate(EasterDate, EasterMonth);
    
    // Julian (East)
    K = -3; E = -1;
    ComputeEasterDate(yer, K, E, &PaschalFullMoon, &EasterDate, &EasterMonth);
    JulianToGregorian(&EasterDate, &EasterMonth); // add 13 days
    lcd.setCursor(14, ii); LcdDate(EasterDate, EasterMonth);
    ii++;
  }
}
oldMinute = minuteGPS;
}


/*****
Purpose: Menu item
Shows local time as an arithmetic computation
Inspired by https://www.albertclock.com/, named after Albert Einstein. Website says:
"This digital clock keeps your brain active and helps to improve 
the mathematical skills of you and your kids in a playful way. Simply by reading the time."

Argument List: int LevelMath  0 for +
                              1 for - or +
                              2 for *, -, or +
                              3 for /,*,-, or +

Return value: Displays on LCD
*****/

void MathClock(int LevelMath )



{
  int Term1Hr, Term2Hr, Term1Min, Term2Min;
  int rnd, HrMult, MinMult;
  
  //  get local time
  local = now() + utcOffset * 60;
  Hour = hour(local); 
  Minute = minute(local);
  Seconds = second(local);

  int SecondPeriod = 60/MATH_PER_MINUTE; // 15; // 1...6 in AlbertClock app

  if (Seconds%SecondPeriod == 0 | oldMinute == -1)  // ever so often + immediate start
  {
    HrMult = 0;
    MinMult = 0;
  
    if (LevelMath == 0)      // +
    
    {
        MathPlus(Hour, &Term1Hr, &Term2Hr);    
        MathPlus(Minute, &Term1Min, &Term2Min);
    } 
   
    else if (LevelMath == 1) //  - and +
// was - : 4/5 probability, + : 1/5 probability. Now 50/50:
    
    {
      // was 0,1: +, 2,3,4,5: - now 0,1,2 or 3,4,5:
      if (random(0,6) <= 2) MathPlus(Hour, &Term1Hr, &Term2Hr);    
      else                  MathMinus(Hour, &Term1Hr, &Term2Hr);    
      
      if (random(0,6) <= 2) MathPlus(Minute, &Term1Min, &Term2Min);
      else                  MathMinus(Minute, &Term1Min, &Term2Min);
    }
    
    else if (LevelMath == 2) // x - +
    // was x: 50% prob, -: 3/8 prob, +: 1/8 prob. Now 1/3, 1/3, 1/3:
    
    {
      rnd = random(0,3); // 0, 1, 2 was [0,...,7] - Hour
      if (rnd == 0)       
      {
        MathMultiply(Hour, &Term1Hr, &Term2Hr);
        HrMult = 1;
      }
      else if (rnd == 1)  MathPlus(Hour, &Term1Hr, &Term2Hr); 
      else                MathMinus(Hour, &Term1Hr, &Term2Hr); 

      rnd = random(0,3); // Minute
      if (rnd == 0)      
      {
        MathMultiply(Minute, &Term1Min, &Term2Min);
        MinMult = 1;
      }
      else if (rnd == 1) MathPlus(Minute, &Term1Min, &Term2Min); 
      else               MathMinus(Minute, &Term1Min, &Term2Min);       
    }
    
    else if (LevelMath == 3) 
    //was  /: 50% prob; x: 30% prob; -: 20% prob, no +, now 3 x 1/3 prob
       
    {
      rnd = random(0,3); //  Hour
      if (rnd == 0)       
      {
        MathDivide(Hour, &Term1Hr, &Term2Hr);
        HrMult = 2;  // means divide
      }
      else if (rnd == 1)
      {
        MathMultiply(Hour, &Term1Hr, &Term2Hr);
        HrMult = 1;
      }
      else  MathMinus(Hour, &Term1Hr, &Term2Hr); 


      rnd = random(0,3); // Minute
      if (rnd == 0)      
      {
        MathDivide(Minute, &Term1Min, &Term2Min);
        MinMult = 2;
      }
      else if (rnd == 1)
      {
        MathMultiply(Minute, &Term1Min, &Term2Min);
        MinMult = 1;
      }
      else  MathMinus(Minute, &Term1Min, &Term2Min);  
    }

    

    // display result on lcd

    // Hour:  
      lcd.setCursor(5, 0); PrintFixedWidth(lcd, Term1Hr, 2); 
      if (HrMult == 2)  // divide
      {
        lcd.print(" ");lcd.print(MATH_CLOCK_DIVIDE);lcd.print(" ");
      }
      else if (HrMult == 1) 
      {
        lcd.print(" ");lcd.print(MATH_CLOCK_MULTIPLY);lcd.print(" ");
      }
      else
      { 
        if (Term2Hr < 0) lcd.print(" - "); else lcd.print(" + "); 
      }
      lcd.print(abs(Term2Hr)); lcd.print("   "); 
      lcd.setCursor(19, 0);lcd.print("h");     

    // Minute:
      lcd.setCursor(8, 2); PrintFixedWidth(lcd, Term1Min, 2);  
      if (MinMult == 2)  // divide
      {
        lcd.print(" ");lcd.print(MATH_CLOCK_DIVIDE);lcd.print(" ");
      }
      else if (MinMult == 1) 
      {
        lcd.print(" ");lcd.print(MATH_CLOCK_MULTIPLY);lcd.print(" ");
      }
      else
      { 
        if (Term2Min < 0) lcd.print(" - "); else lcd.print(" + "); 
      }
      lcd.print(abs(Term2Min)); lcd.print("   ");
      lcd.setCursor(19, 2);lcd.print("m");  
   }

  // lower left corner: show symbols  
  lcd.setCursor(0, 3); 
  if (LevelMath == 3)
  {
    lcd.print(MATH_CLOCK_DIVIDE);lcd.print(MATH_CLOCK_MULTIPLY);lcd.print("-");
  }
  else if (LevelMath == 2) 
  {
    lcd.print(MATH_CLOCK_MULTIPLY);lcd.print("+-");
  }
  else if (LevelMath == 1) lcd.print("+- ");  
  else                     lcd.print("+  ");  

  lcd.setCursor(18, 3); lcd.print("  ");

  oldMinute = minuteGPS;
}

/*****
Purpose: Menu item
Finds the Lunar eclipses for the next years

Argument List: none

Return value: Displays on LCD

*****/

void LunarEclipse()
{
  int pDate[10]; // max 5 per day: packed date = 100*month + day, i.e. 1209 = 9 December
  int eYear[10];
  int pday, pmonth, yy;
  int i;

if (minuteGPS != oldMinute) {

  
  lcd.setCursor(0, 0); lcd.print("Lunar Eclipses ");

  // Good up to and including 2021, but misses the almost eclipse 18-19 July 2027
  // Test: try 2028 with 3 eclipses, see https://www.timeanddate.com/eclipse/list-lunar.html
  yy = yearGPS;//
   
  #ifdef FEATURE_SERIAL_LUNARECLIPSE
    Serial.print((int)yy); Serial.println(F(" ****************"));
  #endif

  for (i=0; i<10; i++) pDate[i]=0;
  
  MoonEclipse(yy, pDate, eYear); 
  int lineNo = 1;
  lcd.setCursor(2,lineNo);lcd.print(yy);lcd.print(":");
  int col = 8;
   
  for (i=0; (i<10 & pDate[i] != 0) ; i++)
  {   
      if (eYear[i] == yy)
      {
        if (col>14)
        {
          col = 2; lineNo = lineNo + 1; // start another line if more than 3 eclipses this year (first time in 2028!)
        } 
        lcd.setCursor(col,lineNo);
        pmonth  = pDate[i] / 100;
        pday = pDate[i] - 100 * pmonth;
        LcdDate(pday, pmonth); lcd.print(" ");  
        col = col+6;
      }
  }
  
  for (i=0; i<10; i++) pDate[i]=0;
  yy = yy + 1;
  MoonEclipse(yy, pDate, eYear);

  //pDate[2] = 729; //pDate[3] = 1209; pDate[4] = 101; // artificial data  for testing
   
  lineNo = lineNo + 1;
  lcd.setCursor(2,lineNo);lcd.print(yy);lcd.print(":");
  col = 8;
  for (i=0; (i < 10 & pDate[i] != 0 & lineNo < 4) ; i++)
  {   
      if (eYear[i] == yy)
      {
        if (col>14)
        {
          col = 2; lineNo = lineNo+1;
        } 
        lcd.setCursor(col,lineNo);
        pmonth  = pDate[i] / 100;
        pday = pDate[i] - 100 * pmonth;
        LcdDate(pday, pmonth); lcd.print(" ");
        col = col+6; 
      }   
  }

  if (lineNo < 3)
  { 
    for (i=0; i<10; i++) pDate[i]=0;

    yy = yy + 1;
    MoonEclipse(yy, pDate, eYear);
   
    lineNo = lineNo + 1;
    lcd.setCursor(2,lineNo);lcd.print(yy);lcd.print(":");
    col = 8;
      
    for (i=0; (i < 2 & pDate[i] != 0 & lineNo < 4) ; i++) // never room for more than two
    {   
        if (eYear[i] == yy)
        {    
          lcd.setCursor(col,lineNo);
          pmonth  = pDate[i] / 100;
          pday = pDate[i] - 100 * pmonth;
          LcdDate(pday, pmonth); lcd.print(" ");
          col = col+6; 
        }   
    }
  }
  if (col < 14) 
  {
    lcd.setCursor(18,3); lcd.print("  ");  // erase lower right-hand corner if not already done
  }
  oldMinute = Minute;
}
}


/*****
Purpose: Menu item
Finds local time with Roman numerals (letters)

Argument List: none

Return value: Displays on LCD
*****/

void Roman()
{
  char RomanOnes[10][6]     = {{"     "}, {"I    "}, {"II   "}, {"III  "}, {"IV   "}, {"V    "}, {"VI   "}, {"VII  "}, {"VIII "}, {"IX   "}}; // left justified
  char RomanTens[6][4]      = {{""}, {"X"}, {"XX"}, {"XXX"}, {"XL"}, {"L"}};  
  int ones, tens;
  char textbuf[21];

  /* The longest symbol
   *  Hours:            18: XVIII, 23: XXIII
   *  Minutes, seconds: 38: XXXVIII
   *  Longest symbol is 5+1+7+1+7 = 21 letters long, so it doesn't fit a single line on a 20 line LCD
   */
 
  //  get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  ones = Hour % 10; tens = (Hour - ones) / 10;
  lcd.setCursor(0, 0); lcd.print(RomanTens[tens]); lcd.print(RomanOnes[ones]);
  //lcd.setCursor(5, 0);lcd.print(HOUR_SEP);

  ones = Minute % 10; tens = (Minute - ones) / 10;
  lcd.setCursor(6, 1); 
  lcd.print(RomanTens[tens]); lcd.print(RomanOnes[ones]);
  //lcd.setCursor(13, 0); lcd.print(MIN_SEP);

  ones = Seconds % 10; tens = (Seconds - ones) / 10;
  lcd.setCursor(12, 2); 
  lcd.print(RomanTens[tens]); lcd.print(RomanOnes[ones]);

//  help screen with normal clock for 0...SECONDS_CLOCK_HELP seconds per minute
  if (Seconds < SECONDS_CLOCK_HELP)  // show time in normal numbers
  {
    lcd.setCursor(0, 3);
    sprintf(textbuf, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
    lcd.print(textbuf);
  }
  else
  {
    lcd.setCursor(0, 3);lcd.print("        ");
  }
  lcd.setCursor(18, 3); lcd.print("  "); // blank out number in lower right-hand corner 
}



/*****
Purpose: Menu item
Calculates local sidereal time based on this calculation, http://www.stargazing.net/kepler/altaz.html 
code pieces from https://hackaday.io/project/163103-freeform-astronomical-clock
also computes local solar time

Argument List: none

Return value: Displays on LCD

Issues:  Is there enough precision? since Mega doesn't have double???
         seems so: accurate within a second or so with http://www.jgiesen.de/astro/astroJS/siderealClock/
*****/

void Sidereal()  // LST - Local Sidereal Time
{
     // 

    double LST_hours,LST_degrees;

    float jd = now() / 86400.0; // cdn(now()); // now/86400, i.e. no of days since 1970
    float j2000 = jd - 10957.5; // 1- line

    double decimal_time = hourGPS + (minuteGPS/60.0)+(secondGPS/3600.0) ;
    double LST = 100.46 + 0.985647 * j2000 + gps.location.lng() + 15*decimal_time; 
    LST_degrees = (LST-(floor(LST/360)*360));
    LST_hours = LST_degrees/15;
    
    int rHours =(int) LST_hours;
    int rMinutes = ((int)floor((LST_hours-rHours)*60));
  
    // compute local solar time based on Equation Of Time
    // EQUATIO: Sidereal & Solar Clock, by Wooduino
    // Routines from http://woodsgood.ca/projects/2015/06/14/equatio-sidereal-solar-clock/

    double tv; // time variable offset in minutes
    doEoTCalc(&tv);

    // time correction factor: https://www.pveducation.org/pvcdrom/properties-of-sunlight/solar-time
    // note 4 minutes = 1 degree of rotation of earth

    // 2021-11-25 test with average of the three
    // rel https://fate.windada.com/cgi-bin/SolarTime_en: 35 sec too fast
    // rel to http://www.jgiesen.de/astro/astroJS/siderealClock/: 51 sec too slow

    // new test with Wikipedia method:
    // rel https://fate.windada.com/cgi-bin/SolarTime_en: 35 sec too fast: 2 sec 3-4 sec faster
    
    
    double tc = 4.0*lon + tv; // correction in minutes: Deviation from center of time zone + Equation of Time
 
   // display results:

    LcdShortDayDateTimeLocal(0, 0); // line 3 local time 
    //lcd.setCursor(0,1);lcd.print("Local");
    
    lcd.setCursor(0,2);lcd.print("  solar");

    char textBuffer[12];
    lcd.setCursor(11,2);

    time_t solar;
    solar = now() + (int)(tc*60);
    Hour = hour(solar); 
    Minute = minute(solar);
    Seconds = second(solar);
    
    //sprintf(textBuffer, " %02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds); 
    // drop seconds:
    sprintf(textBuffer, " %02d%c%02d%   ", Hour, HOUR_SEP, Minute); 
    lcd.print(textBuffer);

    lcd.setCursor(0,3);lcd.print("  sidereal");
    lcd.setCursor(12,3);
    PrintFixedWidth(lcd, rHours, 2,'0'); lcd.print(HOUR_SEP);
    PrintFixedWidth(lcd, rMinutes, 2,'0');

// sidereal calc # 2

//    double localsidereal = localSiderealTime(lon, jd, float(utcOffset) / 60.0); // in radians
//    localsidereal = 24.0 * localsidereal/(2.0*PI); // in hours
//
//    rHours =(int) localsidereal;
//    rMinutes = ((int)floor((localsidereal-rHours)*60));
//    lcd.setCursor(0,3);
//    PrintFixedWidth(lcd, rHours, 2, '0'); lcd.print(HOUR_SEP);
//    PrintFixedWidth(lcd, rMinutes, 2, '0');
//    shows 04:25 when the first one shows 09:36 ???
//    
    
    lcd.setCursor(18,3);lcd.print("  ");    
}

/*****
Purpose: Menu item
Finds local time in Morse code

Argument List: none

Return value: Displays on LCD
*****/

void Morse()   // time in Morse code on LCD
{ 
  int ones, tens;
  char textbuf[21];
  
  //  get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  ones = Hour % 10; tens = (Hour - ones) / 10;
  lcd.setCursor(0, 0); 
  LcdMorse(tens);
//  if (tens != 0) LcdMorse(tens);
//  else lcd.print("     ");
  lcd.print(" ");
  LcdMorse(ones);
  
  ones = Minute % 10; tens = (Minute - ones) / 10;
  lcd.setCursor(4, 1); 
  LcdMorse(tens);
  lcd.print(" ");
  LcdMorse(ones);
  
  ones = Seconds % 10; tens = (Seconds - ones) / 10;
  lcd.setCursor(8, 2); 
  LcdMorse(tens);
  lcd.print(" ");
  LcdMorse(ones);


//  help screen with normal clock for 0...SECONDS_CLOCK_HELP seconds per minute
  if (Seconds < SECONDS_CLOCK_HELP)  // show time in normal numbers
  {
    lcd.setCursor(0, 3);
    sprintf(textbuf, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
    lcd.print(textbuf);
  }
  else
  {
    lcd.setCursor(0, 3);lcd.print("        ");
  }
  lcd.setCursor(18, 3); lcd.print("  "); // blank out number in lower right-hand corner 
}

/*****
Purpose: Menu item
Finds local time written out with words

Argument List: none

Return value: Displays on LCD
*****/

void WordClockEnglish()
{
  char WordOnes[10][6] = {{" oh  "}, {"one  "}, {"two  "}, {"three"}, {"four "}, {"five "}, {"six  "}, {"seven"}, {"eight"}, {"nine "}}; // left justified
  char CapiOnes[10][6] = {{"Oh   "}, {"One  "}, {"Two  "}, {"Three"}, {"Four "}, {"Five "}, {"Six  "}, {"Seven"}, {"Eight"}, {"Nine "}}; // left justified
  char WordTens[6][7]  = {{"    Oh"}, {"   Ten"}, {"Twenty"}, {"Thirty"}, {" Forty"}, {" Fifty"}}; 
  char Teens[10][10]   = {{"         "},{"Eleven   "},{"Twelve   "},{"Thirteen "}, {"Fourteen "}, {"Fifteen  "}, {"Sixteen  "}, {"Seventeen"}, {"Eighteen "}, {"Nineteen "}};
  int ones, tens;
  char textbuf[21];

  /* The longest symbol
   *  Hours:            xx: ?? 
   *  Minutes, seconds: 37: Thirty-seven
   *  Longest symbol is 5+1+7+1+7 = 21 letters long, so it doesn't fit a single line on a 20 line LCD
   */
 
  //  get local time
  local = now() + utcOffset * 60;
  Hour = hour(local);
  Minute = minute(local);
  Seconds = second(local);

  lcd.setCursor(0, 0); 
  if (Hour < 10) 
  {
    lcd.print(CapiOnes[int(Hour)]); lcd.print("      ");
  }
  else if (Hour > 10 & Hour < 20) lcd.print(Teens[int(Hour)-10]);
  else
  {
    ones = Hour % 10; tens = (Hour - ones) / 10;
    lcd.print(WordTens[tens]); 
    if (ones != 0) 
    {
      if (tens!=0) lcd.print("-");
      else lcd.print(" ");
      lcd.print(WordOnes[ones]);
    } 
    else 
    {
      if (tens == 0) lcd.print(WordOnes[ones]); 
      lcd.print("      ");
    }
  }
  
  lcd.setCursor(2,1); 
  if (Minute > 10 & Minute < 20) lcd.print(Teens[int(Minute)-10]);
  else
  {
    ones = Minute % 10; tens = (Minute - ones) / 10;
    lcd.print(WordTens[tens]); 
    if (ones != 0) 
    {
      if (tens!=0) lcd.print("-"); 
      else lcd.print(" "); 
      lcd.print(WordOnes[ones]);
    }
     else 
    {
      if (tens == 0) lcd.print(WordOnes[ones]); 
      lcd.print("      ");
    }
  }
  
  
  lcd.setCursor(4, 2); 
  if (Seconds > 10 & Seconds < 20) lcd.print(Teens[int(Seconds)-10]);
  else
  {
    ones = Seconds % 10; tens = (Seconds - ones) / 10;
    lcd.print(WordTens[tens]); 
    if (ones != 0) 
    {
      if (tens!=0) lcd.print("-");
      else lcd.print(" "); 
      lcd.print(WordOnes[ones]);
    }
    else 
    {
      if (tens == 0) lcd.print(WordOnes[ones]); 
      lcd.print("      ");
    }
  }
  
   lcd.setCursor(0, 3);lcd.print("        ");
   lcd.setCursor(18, 3); lcd.print("  "); // blank out number in lower right-hand corner 
}

/*****
Purpose: Menu item
Run a demo by cycling through all specified memu items

Argument List: int inDemo: 0 or 1
                           1 - first time in demo mode, initialization
                           0 - already in demo mode

Return value: Displays on LCD

Issues: may take up to DWELL_TIME_DEMO seconds before first screen is shown (typ 10-15 sec max)
*****/

void DemoClock(int inDemo // 0 or 1
)  
{   
    if (inDemo == 0) // already in demo mode
        menuSystem(demoDispState, 1);
    else             // first time in demo mode
     {
        lcd.setCursor(0,0); lcd.print("D E M O"); 
        lcd.setCursor(0,1); lcd.print("Multi Face GPS Clock");
        lcd.setCursor(0,3); lcd.print("github/la3za");
     }
}


/*****
Purpose: 
Selects WordClock language 

Argument List: none

Return value: none
*****/

void WordClock()
{
  #if FEATURE_NATIVE_LANGUAGE == 'no'
    WordClockNorwegian();
  #else
    WordClockEnglish();
  #endif
}

/*****
Purpose: Menu item
Shows info about 3 outer and 2 inner planets + alternates between solar/lunar info

Argument List: inner = 1 for inner planets, else outer planets

Return value: Displays on LCD
*****/

void PlanetVisibility(int inner // inner = 1 for inner planets, all other values -> outer
) {
//
// Agrees with https://www.heavens-above.com/PlanetSummary.aspx?lat=59.8348&lng=10.4299&loc=Unnamed&alt=0&tz=CET
// except for brightness or magnitude of mercury: this code says 0.2 when web says 0.6
  
  // Julian day ref noon Universal Time (UT) Monday, 1 January 4713 BC in the Julian calendar:
  //jd = get_julian_date (20, 1, 2017, 17, 0, 0);//UTC
  Seconds = second(now());
  Minute = minute(now());
  Hour = hour(now());
  Day = day(now());
  Month = month(now());
  Year = year(now());
   
  jd = get_julian_date (Day, Month, Year, Hour, Minute, Seconds); // local
 
  #ifdef FEATURE_SERIAL_PLANETARY
    Serial.println("JD:" + String(jd, DEC) + "+" + String(jd_frac, DEC)); // jd = 2457761.375000;
  #endif
  
  get_object_position (2, jd, jd_frac);//earth -- must be included always
  
  lcd.setCursor(0, 0); // top line ********* 
  lcd.print("    El");lcd.write(DEGREE);lcd.print(" Az");lcd.write(DEGREE);lcd.print("   % Magn");
  
  if (inner==1){
    get_object_position (0, jd, jd_frac);
    lcd.setCursor(0, 2);
    lcd.print("Mer "); LCDPlanetData(altitudePlanet, azimuthPlanet, phase, magnitude);
    
    lcd.setCursor(0, 3);
    get_object_position (1, jd, jd_frac);
    lcd.print("Ven "); LCDPlanetData(altitudePlanet, azimuthPlanet, phase, magnitude);

    lcd.setCursor(0,1); 
    if ((now() / 10) % 2 == 0)   // change every 10 seconds
      { 
// Moon
      float Phase, PercentPhase;
      lcd.print("Lun ");
      UpdateMoonPosition();
      MoonPhase(Phase, PercentPhase);
      LCDPlanetData(moon_elevation, moon_azimuth, PercentPhase/100., -12.7);
    }
    else
    {
// Sun
      lcd.print("Sun ");

      /////// Solar elevation //////////////////
      
      cTime c_time;
      cLocation c_loc;
      cSunCoordinates c_sposn;
      double dElevation;
      double dhNoon, dmNoon;
    
      c_time.iYear = yearGPS;
      c_time.iMonth = monthGPS;
      c_time.iDay = dayGPS;
      c_time.dHours = hourGPS;
      c_time.dMinutes = minuteGPS;
      c_time.dSeconds = secondGPS;
    
      c_loc.dLongitude = lon;
      c_loc.dLatitude  = latitude;
    
      c_sposn.dZenithAngle = 0;
      c_sposn.dAzimuth = 0;
    
      float sun_azimuth = 0;
      float sun_elevation = 0;
    
      sunpos(c_time, c_loc, &c_sposn);
    
      // Convert Zenith angle to elevation
      sun_elevation = 90. - c_sposn.dZenithAngle;
      sun_azimuth = c_sposn.dAzimuth;

      LCDPlanetData(round(sun_elevation), round(sun_azimuth), 1., -26.7);

    }
  
  }
  else  // outer planets
  {
    get_object_position (3, jd, jd_frac);
    lcd.setCursor(0, 1);
    lcd.print("Mar "); LCDPlanetData(round(altitudePlanet), round(azimuthPlanet), phase, magnitude);
    
    get_object_position (4, jd, jd_frac);
    lcd.setCursor(0, 2);
    lcd.print("Jup "); LCDPlanetData(round(altitudePlanet), round(azimuthPlanet), phase, magnitude);
    
    get_object_position (5, jd, jd_frac);
    lcd.setCursor(0, 3);
    lcd.print("Sat "); LCDPlanetData(round(altitudePlanet), round(azimuthPlanet), phase, magnitude);

    if (full) get_object_position (6, jd, jd_frac); // Uranus
    if (full) get_object_position (7, jd, jd_frac); // Neptune
  }
}  

/*****
Purpose: Menu item
Shows local time in 4 different calendars: Gregorian (Western), Julian (Eastern), Islamic, Hebrew

Argument List: inner = 1 for inner planets, else outer planets

Return value: Displays on LCD

Issues: Hebrew calendar is quite slow (5+ seconds) on an Arduino Mega
*****/

void ISOHebIslam() {     // ISOdate, Hebrew, Islamic

//#ifdef FEATURE_FAKE_SERIAL_GPS_IN
//      hourGPS = hour(now());
//      minuteGPS = minute(now());
//      secondGPS = second(now());
//#endif

#ifdef FEATURE_DATE_PER_SECOND   // for stepping date quickly and check calender function
    local = now() + utcOffset * 60 + dateIteration*86400; // fake local time by stepping up to 1 sec/day
    dateIteration = dateIteration + 1;
//  Serial.print(dateIteration); Serial.print(": ");
//  Serial.println(local);
#endif

// algorithms in Nachum Dershowitz and Edward M. Reingold, Calendrical Calculations,
// Software-Practice and Experience 20 (1990), 899-928
// code from https://reingold.co/calendar.C

       lcd.setCursor(0, 0); // top line *********
       // all dates are in local time
       GregorianDate a(month(local), day(local), year(local));      
       LcdDate(a.GetDay(), a.GetMonth(), a.GetYear());
//
////        Serial.print("Absolute date ");Serial.println(a);

        lcd.setCursor(10, 0);
        IsoDate ISO(a);  
        lcd.print(" Week   "); lcd.print(ISO.GetWeek());
               
        lcd.setCursor(0, 1);
        JulianDate Jul(a);
        LcdDate(Jul.GetDay(), Jul.GetMonth(), Jul.GetYear());
        lcd.print(" Julian");

        bool Byz = false; //true; - works best if date has year as last item (= EU/US)
        if (Byz)
        {
            lcd.setCursor(10, 1);     
        //    Byzantine year = Annus Mundi rel to September 1, 5509 BC
        //    used by the Eastern Orthodox Church from c. 691 to 1728 https://en.wikipedia.org/wiki/Byzantine_calendar
            int ByzYear = Jul.GetYear() + 5508;
            if (Jul.GetMonth()>= 9) ByzYear = ByzYear + 1;
            lcd.print("/");lcd.print(ByzYear);
            lcd.print("  J/B");
          }       

        lcd.setCursor(0, 2);
        char IslamicMonth[12][10] = {{"Muharram "}, {"Safar    "}, {"Rabi I   "}, {"Rabi II  "}, {"Jumada I "}, {"Jumada II"},{"Rajab    "}, {"Sha'ban  "}, {"Ramadan  "}, {"Shawwal  "}, {"DhuAlQada"}, {"DhuAlHija"}}; // left justified
        IslamicDate Isl(a);
        int m;
        m = Isl.GetMonth();
        LcdDate(Isl.GetDay(), m, Isl.GetYear());    
        lcd.print(" "); lcd.print(IslamicMonth[m-1]); 

 // Hebrew calendar is complicated and *** v e r y *** slow. Therefore it is on the last line:
        
        char HebrewMonth[13][10] = {{"Nisan    "}, {"Iyyar    "}, {"Sivan    "}, {"Tammuz   "}, {"Av       "}, {"Elul     "}, {"Tishri   "}, {"Heshvan  "}, {"Kislev   "}, {"Teveth   "}, {"Shevat   "}, {"Adar     "}, {"Adar II  "}}; // left justified
 
        if (local % 10 == 0)     // only check every x sec. Otherwise no time for the clock to read encoder or buttons
          {
              HebrewDate Heb(a);  
              m = Heb.GetMonth();
              lcd.setCursor(0, 3);
              LcdDate(Heb.GetDay(), m, Heb.GetYear());
              lcd.print(" "); lcd.print(HebrewMonth[m-1]); 
         //     lcd.setCursor(18, 3); lcd.print("  ");
          }       
}


/*****
Purpose: 
Display all kinds of GPS-related info

Argument List: none

Return value: none
*****/ 

 void GPSInfo()
{
  // builds on the example program SatelliteTracker from the TinyGPS++ library
  // https://www.arduino.cc/reference/en/libraries/tinygps/

//float SNRavg = 0;
//int total = 0;

    #ifdef FEATURE_SERIAL_GPS 
      Serial.println(F("GPSInfo"));
    #endif

    
    if (totalGPGSVMessages.isUpdated())
    {

//https://github.com/mikalhart/TinyGPSPlus/issues/52
//     int totalMessages = atoi(totalGPGSVMessages.value());
//     int currentMessage = atoi(messageNumber.value());
//      for (int i = 0; currentMessage < totalMessages ? i < 4 : i < (atoi(satsInView.value()) % 4); ++i)
    
      int totalMessages = atoi(totalGPGSVMessages.value());
      int currentMessage = atoi(messageNumber.value());    
      if (totalMessages == currentMessage)
      {   
      #ifdef FEATURE_SERIAL_GPS 
        Serial.print(F("Sats in use = ")); Serial.print(gps.satellites.value());
        Serial.print(F(" Nums = "));
      
        for (int i=0; i<MAX_SATELLITES; ++i)
        {
          if (sats[i].active)
          {
            Serial.print(i+1);
            Serial.print(F(" "));
          }
        }
      #endif
       
        int total = 0;
        float SNRavg = 0;
        #ifdef FEATURE_SERIAL_GPS 
          Serial.print(F(" SNR = "));
        #endif
        
        for (int i=0; i<MAX_SATELLITES; ++i)
        {
          if (sats[i].active)
          {
            #ifdef FEATURE_SERIAL_GPS 
                Serial.print(sats[i].snr);
                Serial.print(F(" "));
            #endif
            if (sats[i].snr >0)          // 0 when not tracking
            {
              total = total + 1; 
              SNRavg = SNRavg + float(sats[i].snr);
            }
          }
        }
       
          lcd.setCursor(0,0); lcd.print("In view "); //printFixedWidth(lcd, satsInView.value(), 2);
          if (satsInView.value()<10) lcd.print(" ");
          lcd.print(satsInView.value()); lcd.print(" Sats");
          
          noSats = gps.satellites.value();  // in list of http://arduiniana.org/libraries/tinygpsplus/
          lcd.setCursor(0,1); lcd.print("In fix  "); //printFixedWidth(lcd, noSats, 2); 
          if (noSats<10) lcd.print(" ");
          lcd.print(noSats);
 
          SNRavg = SNRavg/total; 
          lcd.print(" SNR "); //printFixedWidth(lcd, (int)SNRavg),2);
          if ((int)SNRavg<10) lcd.print(" ");
          lcd.print((int)SNRavg); lcd.print(" dB");
          lcd.setCursor(0,2); lcd.print("Mode    "); lcd.print(GPSMode.value());lcd.print("D Status  ");
          lcd.print(posStatus.value()); 
          
          float hdop   = gps.hdop.hdop();  // in list of http://arduiniana.org/libraries/tinygpsplus/
          lcd.setCursor(0,3); lcd.print("Hdop  "); lcd.print(hdop); 
          if      (hdop<1) lcd.print(" Ideal    ");// 1-2 Excellent, 2-5 Good https://en.wikipedia.org/wiki/Dilution_of_precision_(navigation)
          else if (hdop<2) lcd.print(" Excellent");
          else if (hdop<5) lcd.print(" Good     ");
          else             lcd.print(" No good  ");
       #ifdef FEATURE_SERIAL_GPS 
          Serial.print(F(" Total=")); Serial.print(total);
          Serial.print(F(" InView=")); Serial.print(satsInView.value());       
              
          Serial.print(F(" SNRavg=")); Serial.print((int)SNRavg);
          Serial.print(F(" Mode = ")); Serial.print(GPSMode.value()); // 1-none, 2=2D, 3=3D
          Serial.print(F(" Status = ")); Serial.print(posStatus.value()); // A-valid, V-invalid
          Serial.println();
        #endif

          for (int i=0; i<MAX_SATELLITES; ++i)
            sats[i].active = false; 
    }    
  }
 
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// End of functions for Menu system ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/// THE END ///
