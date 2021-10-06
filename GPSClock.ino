#define CODE_VERSION "1.02 2021-10-06"

/*
LA3ZA GPS Clock
    
Copyright 2015 - 2021 Sverre Holm, LA3ZA
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
           Controlled by a GPS module outputting data over an RS232 serial interface, typ. QRPLabs QLG1 GPS Receiver kit, or the QLG2
           GPS is handled with the TinyGPS++ library
           Shows raw GPS data such as UTC time and date, position, altitude, and number of satellitess
           Also with various forms of binary, BCD, digit-5, digit-10 displays
           Shows derived GPS data such as 6-digit locator
           Finds local time and handles daylight saving automatically using the Timezone library
           Finds local sunset and sunrise, either actual value, or civil, nautical, or astronomical using the Sunrise library
           The clock also gives local solar height based on the Sunpos library from the K3NG rotator controller.
           The clock also provides the lunar phase as well as predict necxt rise/set time for the moon


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

/* Functions in this file:

            setup
            loop
            
            LocalUTC
            UTCLocator
            
            LocalSun
            LocalSunMoon
            LocalMoon
            MoonRiseSet
            
            Binary
            Bar
            MengenLehrUhr
            LinearUhr
            
            InternalTime
            code_Status
            
            UTCPosition
            
            NCDXFBeacons
            WSPRsequence

            HexOctalClock() [added 29.09.2021]
*/
/*
Revisions:
          

          1.02  06.10.2021
                - UTC and position screen: Changed layout. Now handles Western longitudes and Southern latitudes also. Thanks Mitch W4OA
                - Corrected bug in Maidenhead routine, only appeared if letter 5 was beyond a certain letter in the alphabet. Thanks Ross VA1KAY

          1.01  29.09.2021 
                - Fixed  small layout bug on screen
                - New variable in clock_options.h: SECONDS_CLOCK_HELP - no of seconds to show normal clock in binary, BCD, hex, octal etc clocks.
                - 2 new screens: no 19 and 20: hex and octal clock
                
          1.0   24.09.2021
                First public release
                18 different screens                
          
*/

///// load user-defined setups //////

#include "clock_debug.h"            // debugging options via serial port
#include "clock_options.h"        // customization of order and number of menu items
#include "clock_pin_settings.h"     // hardware pins 

//////////////////////////////////////

// libraries

#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time - timekeepng functionality
#include <Timezone_Generic.h>   // https://github.com/khoih-prog/Timezone_Generic

#include "clock_zone.h"         // user-defined setup for local time zone and daylight saving

#include <TinyGPS++.h>          // http://arduiniana.org/libraries/tinygpsplus/

#if defined(FEATURE_LCD_I2C)
  #include <Wire.h>               // For I2C. Comes with Arduino IDE
  #include <LiquidCrystal_I2C.h>  // Install NewliquidCrystal_1.3.4.zip https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/
#endif

#if defined(FEATURE_LCD_4BIT)
  #include <LiquidCrystal.h>
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

#define RAD                 (PI/180.0)
#define SMALL_FLOAT         (1e-12)

#define DashedUpArrow       1
#define DashedDownArrow     2
#define UpArrow             3
#define DownArrow           4
#define DegreeSymbol        223


int dispState ;  // depends on button, decides what to display
byte wkday;
char today[10];
double latitude, lon, alt;
int Year;
byte Month, Day, Hour, Minute, Seconds;
u32 noSats;

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;
time_t prevDisplay = 0;     // when the digital clock was displayed

int packedRise;
double moon_azimuth = 0;
double moon_elevation = 0;
double moon_dist = 0;
float ag; // age of moon in days

int iiii;
int oldminute = -1;

int yearGPS;
uint8_t monthGPS, dayGPS, hourGPS, minuteGPS, secondGPS, weekdayGPS;

int val; // pot value which controls backlight brighness
int menuOrder[30]; //menuOrder[noOfStates];


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
  lcd.createChar(DashedUpArrow, upDashedArray);
  lcd.createChar(DashedDownArrow, downDashedArray);
  lcd.createChar(UpArrow, upArray);
  lcd.createChar(DownArrow, downArray);
  #ifdef FEATURE_DAY_NAME_NATIVE
    #include "clock_native.h"  // user customable character set
  #endif
  lcd.clear(); // in order to set the LCD back to the proper memory mode

  pinMode(LCD_pwm, OUTPUT);
  digitalWrite(LCD_pwm, HIGH);   // sets the backlight LED to full

  // unroll menu system order
  for (iiii = 0; iiii < noOfStates; iiii += 1) menuOrder[menuIn[iiii]] = iiii;

  code_Status();  // start screen
  lcd.setCursor(10, 2); lcd.print("......"); // ... waiting for GPS
  lcd.setCursor(0, 3); lcd.print("        ");
    delay(1000);

  Serial1.begin(GPSBaud);

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
}

  ////////////////////////////////////// L O O P //////////////////////////////////////////////////////////////////
  void loop() {
    val = analogRead(potentiometer);   // read the input pin for control of backlight
    // analogRead values go from 0 to 1023, analogWrite values from 0 to 255
    // compress using sqrt to get smoother characteristics for the eyes
    analogWrite(LCD_pwm, (int)(255 * sqrt((float)val / 1023))); //

    byte button = analogbuttonread(0); // using K3NG function
    if (button == 2) { // increase menu # by one
      dispState = (dispState + 1) % noOfStates;
      lcd.clear();
      oldminute = -1; // to get immediate display of some info
      lcd.setCursor(18, 3); lcd.print(dispState); // lower left-hand corner
      delay(300); // was 300
    }
    else if (button == 1) { // decrease menu # by one
      dispState = (dispState - 1) % noOfStates;;
      lcd.clear();
      oldminute = -1; // to get immediate display of some info
      if (dispState < 0) dispState += noOfStates;
      lcd.setCursor(18, 3); lcd.print(dispState);
      delay(300);
    }

    else {
      while (Serial1.available()) {
        if (gps.encode(Serial1.read())) { // process gps messages
          // when GPS reports new data...
          unsigned long age;

          hourGPS = gps.time.hour();
          minuteGPS = gps.time.minute();
          secondGPS = gps.time.second();
          dayGPS = gps.date.day() ;
          monthGPS = gps.date.month() ;
          yearGPS = gps.date.year() ;
          age = gps.location.age();

          //int utc, int local; // must be time_t and global
          utc = now(); //+(long)86400*150;

          #ifdef AUTO_UTC_OFFSET       
            #include "clock_zone2.h" // this file contains the concrete time zone call
            UTCoffset = local/long(60) - utc/long(60); // order of calculation is important          
          #else 
            local = utc + UTCoffset * 60; // UTCoffset is set in clock_zone.h
          #endif

         #ifdef FEATURE_SERIAL_TIME
            Serial.print(F("utc     "));Serial.println(utc);
            Serial.print(F("local   "));Serial.println(local);
            Serial.print(F("diff,s  "));Serial.println(long(local-utc));
            Serial.print(F("diff,m1 "));Serial.println(long(local-utc)/long(60));
            Serial.print(F("diff,m  "));Serial.println(long(local)/long(60)-long(utc)/long(60));
            
            Serial.print(F("UTCoffset: "));
            Serial.println(UTCoffset);
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

/////////////////////////////////////////////// USER INTERFACE /////////////////////////////////////////////////////////
        #ifdef FEATURE_SERIAL_MENU
          Serial.println(F("menuOrder: "));
          for (iiii = 0; iiii < noOfStates; iiii += 1) Serial.println(menuOrder[iiii]);
          Serial.print(F("dispState ")); Serial.println(dispState);
          Serial.println((dispState % noOfStates));
          Serial.println(menuOrder[dispState % noOfStates]);
        #endif

////////////// Menu system ////////////////////////////////////////////////////////////////////////////////
////////////// This is the order of the menu system unless menuOrder[] contains information to the contrary

          if      ((dispState) == menuOrder[0])  LocalUTC();      // local time, date; UTC, locator
          else if ((dispState) == menuOrder[1])  UTCLocator();    // UTC, locator, # sats

// Sun, moon:  
          else if ((dispState) == menuOrder[2])  LocalSun();      // local time, sun x 3 
          else if ((dispState) == menuOrder[3])  LocalSunMoon();  // local time, sun, moon 
          else if ((dispState) == menuOrder[4])  LocalMoon();     // local time, moon size and elevation
          else if ((dispState) == menuOrder[5])  MoonRiseSet();   // Moon rises and sets at these times

// Nice to have
          else if ((dispState) == menuOrder[6]) TimeZones();     // Other time zones           
 
// Fancy, sometimes near unreadable displays, fun to program, and fun to look at:
          else if ((dispState) == menuOrder[7])  Binary(2);       // Binary, horizontal, display of time
          else if ((dispState) == menuOrder[8])  Binary(1);       // BCD, horizontal, display of time
          else if ((dispState) == menuOrder[9])  Binary(0);       // BCD vertical display of time
          else if ((dispState) == menuOrder[10]) Bar();           // horizontal bar
          else if ((dispState) == menuOrder[11]) MengenLehrUhr(); // set theory clock
          else if ((dispState) == menuOrder[12]) LinearUhr();     // Linear clock        
// debugging:
          else if ((dispState) == menuOrder[13]) InternalTime();  // Internal time - for debugging
          else if ((dispState) == menuOrder[14]) code_Status();   //

// Nice to have:           
          else if ((dispState) == menuOrder[15]) UTCPosition();   // position
          
// WSPR and beacons:
          else if ((dispState) == menuOrder[16]) NCDXFBeacons(2); // UTC + NCDXF beacons, 18-28 MHz
          else if ((dispState) == menuOrder[17]) NCDXFBeacons(1); // UTC + NCDXF beacons, 14-21 MHz
          else if ((dispState) == menuOrder[18]) WSPRsequence();  // UTC + Coordinated WSPR band/frequency (20 min cycle)

          else if ((dispState) == menuOrder[19]) HexOctalClock(0);      // Hex clock
          else if ((dispState) == menuOrder[20]) HexOctalClock(1);      // Octal clock
          
        }
      }
    }
  }  // end loop

////////////////////////////////////// END LOOP //////////////////////////////////////////////////////////////////


// The rest of this file consists of one routine per menu item:

// Menu item ///////////////////////////////////////////////////////////////////////////////////////////
void LocalUTC() { // local time, UTC,  locator

    char textbuffer[11];
    // get local time

    local = now() + UTCoffset * 60;
    Hour = hour(local);
    Minute = minute(local);
    Seconds = second(local);

    lcd.setCursor(0, 0); // top line *********
    sprintf(textbuffer, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
    lcd.print(textbuffer);
    lcd.print("      ");
    // local date
    Day = day(local);
    Month = month(local);
    Year = year(local);
    if (dayGPS != 0)
    {
 
     #ifdef FEATURE_DAY_NAME_NATIVE
        lcd.setCursor(13, 0);
        nativeDayLong(local);
      #else // English
        lcd.setCursor(11, 0);
        sprintf(today, "%09s", dayStr(weekday(local)));
        lcd.print(today); lcd.print(" ");
      #endif
    
    lcd.setCursor(0, 1); //////// line 2


        lcd.print("          ");
        lcd.setCursor(10, 1);

       if (DATEORDER=='B')
          {
            printFixedWidth(lcd, Year, 4); lcd.print(DATE_SEP);
            printFixedWidth(lcd, Month, 2,'0'); lcd.print(DATE_SEP); 
            printFixedWidth(lcd, Day, 2,'0'); 
            }
       else if (DATEORDER=='M')
            {
            printFixedWidth(lcd, Month, 2,'0'); lcd.print(DATE_SEP);
            printFixedWidth(lcd, Day, 2,'0'); lcd.print(DATE_SEP);
            printFixedWidth(lcd, Year, 4);
            }
          {
            printFixedWidth(lcd, Day, 2,'0'); lcd.print(DATE_SEP);
            printFixedWidth(lcd, Month, 2,'0'); lcd.print(DATE_SEP);
            printFixedWidth(lcd, Year, 4);
          }
    }

    lcd.setCursor(0, 2); lcd.print("                    ");
    LcdUTCTimeLocator(3); // / last line *********
    
    oldminute = minuteGPS;
  }

 
// Menu item //////////////////////////////////////////////////////////////////////////////////////////

void UTCLocator() {     // UTC, locator, # satellites
    char textbuffer[25];

    lcd.setCursor(0, 0); // top line *********
    if (gps.time.isValid()) {
      sprintf(textbuffer, "%02d%c%02d%c%02d         UTC", hourGPS, HOUR_SEP, minuteGPS, MIN_SEP, secondGPS);
      lcd.print(textbuffer);
    }

    // UTC date

    if (dayGPS != 0)
    {
      lcd.setCursor(0, 1); // line 1
      lcd.print(dayStr(weekdayGPS)); lcd.print("   "); // two more spaces 14.04.2018

      lcd.setCursor(10, 1); 
      if (DATEORDER=='B')
          {
            printFixedWidth(lcd, yearGPS, 4); lcd.print(DATE_SEP);
            printFixedWidth(lcd, monthGPS, 2,'0'); lcd.print(DATE_SEP); 
            printFixedWidth(lcd, dayGPS,2, '0'); 
            }
       else if (DATEORDER=='M')
            {
            printFixedWidth(lcd, monthGPS, 2,'0'); lcd.print(DATE_SEP); 
            printFixedWidth(lcd, dayGPS,2, '0');  lcd.print(DATE_SEP);
            printFixedWidth(lcd, yearGPS, 4);  
            }
          {
            printFixedWidth(lcd, dayGPS,2, '0');  lcd.print(DATE_SEP);
            printFixedWidth(lcd, monthGPS, 2,'0'); lcd.print(DATE_SEP);
            printFixedWidth(lcd, yearGPS, 4);  
          }
      
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
void LocalSun() { // local time, sun x 3
    //
    // shows Actual (0 deg), Civil (-6 deg), and Nautical (-12 deg) sun rise/set
    //
    LcdShortDayDateTimeLocal(0, 2);  // line 0
    if (gps.location.isValid()) {
      if (minuteGPS != oldminute) {      
        #ifndef DEBUG_MANUAL_POSITION 
        latitude = gps.location.lat();
        lon = gps.location.lng();
      #else
        latitude = latitude_manual;
        lon      = longitude_manual;
      #endif
   
     LcdSolarRiseSet(1,' ');  
     LcdSolarRiseSet(2,'C');
     LcdSolarRiseSet(3,'N');
     }
    }
    oldminute = minuteGPS;
  }


// Menu item //////////////////////////////////////////////////////////////////////////////////
void LocalSunMoon() { // local time, sun, moon
    //
    // shows Actual (0 deg) and Civil (-6 deg) sun rise/set
    //

    LcdShortDayDateTimeLocal(0, 2);  // line 0, time offset 2 to the left

    if (gps.location.isValid()) {
      if (minuteGPS != oldminute) {
        
      #ifndef DEBUG_MANUAL_POSITION 
        latitude = gps.location.lat();
        lon = gps.location.lng();
      #else
        latitude = latitude_manual;
        lon      = longitude_manual;
      #endif

      LcdSolarRiseSet(1,' ');  // line 1
      LcdSolarRiseSet(2,'C'); // line 2
     
// MOON 
        
      lcd.setCursor(0, 3);  // last line
      lcd.print("M ");  
      
      // next rise / set
      short pRise, pSet, pLocal, pTime;
      double rAz, sAz;
      int order;
     
      GetNextRiseSet(&pRise, &rAz, &pSet, &sAz, &order); 
      #ifdef FEATURE_SERIAL_MOON
         Serial.print(F("LocalSunMoon: order: ")); Serial.println(order);
      #endif
   
      local = now() + UTCoffset * 60;
      Hour = hour(local);
      Minute = minute(local);

      int packedTime = Hour*100 + Minute;

      lcd.setCursor(2, 3); // last line
      
      // find next event
      if (order == 1)  // Rise 
      {
        pTime = pRise;  lcd.write(UpArrow); 
      }
      else // Set (or order not initialized correctly)
      {
        pTime = pSet; lcd.write(DownArrow); 
      }
     
      if (pTime > -1)
        {
          int pHr  = pTime / 100;
          int pMin = pTime - 100 * pHr;           
          printFixedWidth(lcd, pHr, 2);lcd.print(HOUR_SEP);printFixedWidth(lcd, pMin,2, '0');lcd.print(" ");    
        }
      else lcd.print(" - ");     

      float PhaseM, PercentPhaseM;     
      //MoonPhaseAccurate(PhaseM, PercentPhaseM);  
      MoonPhase(PhaseM, PercentPhaseM); 

      #ifdef FEATURE_SERIAL_MOON
        Serial.println(F("LocalSunMoon: "));
        Serial.print(F(" PhaseM, PercentPhaseM "));
        Serial.print(PhaseM);Serial.print(F(", "));Serial.println(PercentPhaseM);
      #endif

       lcd.setCursor(9, 3);
       MoonSymbol(PhaseM); // (, O, ), symbol
       MoonWaxWane(PhaseM); //arrow up/down or ' ' (space)

     
       printFixedWidth(lcd, (int)round(PercentPhaseM), 3);
       lcd.print("%");               

       update_moon_position();
       lcd.setCursor(16, 3);
       printFixedWidth(lcd, (int)round(moon_elevation), 3);
       lcd.write(DegreeSymbol); 
       }
    }
    oldminute = minuteGPS;
  }




// Menu item /////////////////////////////////////////////////////////////////////////////////////////////
void LocalMoon() { // local time, moon phase, elevation, next rise/set

    String textbuf;
    float percentage;

    LcdShortDayDateTimeLocal(0,1);  // line 0, moved 1 position left to line up with next lines
    
    if (gps.location.isValid()) {
      if (minuteGPS != oldminute) {  // update display every minute

        // days since last new moon
        float Phase, PercentPhase;
       
        update_moon_position();
        
       lcd.setCursor(0, 3); // line 3
//        
//        lcd.print("  ");
//        textbuf = String(moon_dist, 0);
//        lcd.print(textbuf); lcd.print(" km");

        lcd.print(" ");
        printFixedWidth(lcd, int(round(moon_dist/4067.0)), 3);
        lcd.print("% ");
        
        printFixedWidth(lcd, int(round(moon_dist/1000.0)), 3);
        lcd.print("'km ");
            
        
        MoonPhase(Phase, PercentPhase);

        lcd.setCursor(14, 3);            
        MoonSymbol(Phase);  // (,0,)
        MoonWaxWane(Phase); // arrow
        
        lcd.setCursor(16, 3); 
        printFixedWidth(lcd, (int)(abs(round(PercentPhase))), 3);
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
        printFixedWidth(lcd, (int)round(moon_elevation), 4);
        lcd.write(DegreeSymbol); 
        
        lcd.setCursor(13, 1);
        lcd.print("Az ");
        printFixedWidth(lcd, (int)round(moon_azimuth), 3);
        lcd.write(DegreeSymbol); 
        
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
      local = now() + UTCoffset * 60;
      Hour = hour(local);
      Minute = minute(local);

      int packedTime = Hour*100 + Minute;

      // find next event
      if (order == 1)
      // Rise 
      {
        pTime = pRise;  Symb = UpArrow; Az = rAz;
      }
      else
      {
       pTime = pSet; Symb = DownArrow; Az = sAz;  
      }
     
        lcd.setCursor(2, 2); // line 2
 
        if (pTime > -1)
        {
          int pHr  = pTime / 100;
          int pMin = pTime - 100 * pHr;   
           
          lcd.write(Symb);lcd.print("   ");
          printFixedWidth(lcd, pHr, 2);lcd.print(HOUR_SEP);printFixedWidth(lcd, pMin,2, '0');lcd.print(" ");
          lcd.setCursor(13, 2);
          lcd.print("Az "); 
          printFixedWidth(lcd, (int)round(Az), 3);
          lcd.write(DegreeSymbol); 
        }
        else lcd.print("  No Rise/Set       ");   
   
        oldminute = minuteGPS;
      }
    }
  }

// Menu item ////////////////////////////////////////////////////////
void MoonRiseSet(void) {

      if (gps.location.isValid()) {
      
      #ifndef DEBUG_MANUAL_POSITION 
        latitude = gps.location.lat();
        lon = gps.location.lng();
      #else
        latitude = latitude_manual;
        lon      = longitude_manual;
      #endif
      
      if (minuteGPS != oldminute) {  
  
        short pRise, pSet, pRise2, pSet2, pLocal;
        double rAz, sAz, rAz2, sAz2;
  
        // rise/set for this UTC day:
         GetMoonRiseSetTimes(float(UTCoffset)/60.0, latitude, lon, &pRise, &rAz, &pSet, &sAz);
  
        lcd.setCursor(0, 0); // top line
        lcd.print("M "); 
        
        int MoonRiseHr  = pRise / 100;
        int MoonRiseMin = pRise - 100 * MoonRiseHr;
        int MoonSetHr  = pSet / 100;
        int MoonSetMin = pSet - 100 * MoonSetHr;

        if (pRise < pSet) lcd.setCursor(2,0); 
        else              lcd.setCursor(2,1);
  
        lcd.write(UpArrow); lcd.print(" "); 
          
         if (pRise >-1){
          printFixedWidth(lcd, MoonRiseHr, 2,'0');   lcd.print(HOUR_SEP); 
          printFixedWidth(lcd, MoonRiseMin, 2,'0');  lcd.print("  ");
          printFixedWidth(lcd, (int)round(rAz), 4);
          lcd.write(DegreeSymbol); lcd.print("  ");
        }
        else
        {
          lcd.print(pRise);lcd.print("              ");
        }

        if (pRise < pSet) lcd.setCursor(2,1);
        else              lcd.setCursor(2,0); 
        lcd.write(DownArrow);  lcd.print(" "); 
        if (pSet >-1){
          printFixedWidth(lcd, MoonSetHr, 2,'0');   lcd.print(HOUR_SEP); // doesn't handle 00:48 well with ' ' as separator
          printFixedWidth(lcd, MoonSetMin, 2,'0');  lcd.print("  ");
          printFixedWidth(lcd, (int)round(sAz), 4); lcd.write(DegreeSymbol); lcd.print("  ");
        }
        else
        {
          lcd.print(pSet);lcd.print("              ");
        }

       // rise/set for next UTC day:
         GetMoonRiseSetTimes(float(UTCoffset)/60.0 - 24.0, latitude, lon, &pRise2, &rAz2, &pSet2, &sAz2);

// Rise and set times for moon:

        MoonRiseHr  = pRise2 / 100;
        MoonRiseMin = pRise2 - 100 * MoonRiseHr;
        MoonSetHr  = pSet2 / 100;
        MoonSetMin = pSet2 - 100 * MoonSetHr;

        if (pRise2 < pSet2) lcd.setCursor(2,2);
        else              lcd.setCursor(2,3);
  
        lcd.write(UpArrow); lcd.print(" "); 
          
         if (pRise2 >-1){
          printFixedWidth(lcd, MoonRiseHr, 2,'0');   lcd.print(HOUR_SEP); 
          printFixedWidth(lcd, MoonRiseMin, 2,'0');  lcd.print("  ");
          printFixedWidth(lcd, (int)round(rAz2), 4);
          lcd.write(DegreeSymbol); lcd.print("  ");
        }
        else
        {
          lcd.print(pRise2);lcd.print("              ");
        }
  
        if (pRise2 < pSet2) lcd.setCursor(2,3);
        else              lcd.setCursor(2,2);
  
        lcd.write(DownArrow);  lcd.print(" "); 
        if (pSet2 >-1){
          printFixedWidth(lcd, MoonSetHr, 2, '0');   lcd.print(HOUR_SEP); // doesn't handle 00:48 well with ' ' as separator
          printFixedWidth(lcd, MoonSetMin, 2,'0');   lcd.print("  ");
          printFixedWidth(lcd, (int)round(sAz2), 4); lcd.write(DegreeSymbol); lcd.print("  ");
        }
        else
        {
          lcd.print(pSet2);lcd.print("              ");
        }
        lcd.setCursor(18,3);lcd.print("  ");
      }
    }
    oldminute = minuteGPS;
   }




 

// Menu items ///////////////////////////////////////////////////////////////////////////////////////////
void Binary(
  int mode      // mode = 0 - vertical BCD
                // mode = 1 - horizontal BCD
                // mode = 2 - horisontal binary
  ) { // binary local time
    
    char textbuffer[12]; // was [9] -> memory overwrite
    int tens, ones;

    int BinaryTensHour[5], BinaryHour[5], BinaryTensMinute[5], BinaryMinute[5], BinaryTensSeconds[5], BinarySeconds[5];

    // get local time
    local = now() + UTCoffset * 60;
    Hour = hour(local);
    Minute = minute(local);
    Seconds = second(local);

    // convert to BCD

    // must send a variable, not an equation, to decToBinary as it does in-place arithmetic on input variable
    ones = Hour % 10; tens = (Hour - ones) / 10;
    decToBinary(ones, BinaryHour);
    decToBinary(tens, BinaryTensHour); 

    ones = Minute % 10; tens = (Minute - ones) / 10;
    decToBinary(tens, BinaryTensMinute); decToBinary(ones, BinaryMinute);

    ones = Seconds % 10;   tens = (Seconds - ones) / 10;
    decToBinary(tens, BinaryTensSeconds); decToBinary(ones, BinarySeconds);


    if (mode == 0) // vertical digits:
    {
      lcd.setCursor(0, 0); lcd.print("BCD");
      
      lcd.setCursor(9, 0);
      sprintf(textbuffer, " %1d  %1d  %1d", BinaryHour[1], BinaryMinute[1], BinarySeconds[1]);
      lcd.print(textbuffer);
      lcd.setCursor(19,0);lcd.print("8");

   //   lcd.setCursor(0,1); lcd.print("hh mm ss");
      lcd.setCursor(9, 1);
      sprintf(textbuffer, " %1d %1d%1d %1d%1d", BinaryHour[2], BinaryTensMinute[2], BinaryMinute[2], BinaryTensSeconds[2], BinarySeconds[2]);
      lcd.print(textbuffer);
      lcd.setCursor(19,1);lcd.print("4");

      lcd.setCursor(9, 2);
      sprintf(textbuffer, "%1d%1d %1d%1d %1d%1d", BinaryTensHour[3], BinaryHour[3], BinaryTensMinute[3], BinaryMinute[3], BinaryTensSeconds[3], BinarySeconds[3]);
      lcd.print(textbuffer);
      lcd.setCursor(19,2);lcd.print("2");

      
      lcd.setCursor(9, 3); //LSB
      sprintf(textbuffer, "%1d%1d %1d%1d %1d%1d  ", BinaryTensHour[4], BinaryHour[4], BinaryTensMinute[4], BinaryMinute[4], BinaryTensSeconds[4], BinarySeconds[4]);
      lcd.print(textbuffer);
      lcd.setCursor(19,3);lcd.print("1");
    }
    else if (mode == 1)
    {
      //// horizontal BCD digits:

      lcd.setCursor(0, 0); lcd.print("BCD");
      
      lcd.setCursor(9, 1); sprintf(textbuffer, "  %1d%1d ", BinaryTensHour[3], BinaryTensHour[4] );
      lcd.print(textbuffer);
      sprintf(textbuffer, "%1d%1d%1d%1d H", BinaryHour[1], BinaryHour[2], BinaryHour[3], BinaryHour[4]);
      lcd.print(textbuffer);

      lcd.setCursor(9, 2);  sprintf(textbuffer, " %1d%1d%1d ", BinaryTensMinute[2], BinaryTensMinute[3], BinaryTensMinute[4] );
      lcd.print(textbuffer);
      sprintf(textbuffer, "%1d%1d%1d%1d M", BinaryMinute[1], BinaryMinute[2], BinaryMinute[3], BinaryMinute[4] );
      lcd.print(textbuffer);

      lcd.setCursor(9, 3);  sprintf(textbuffer, " %1d%1d%1d ", BinaryTensSeconds[2], BinaryTensSeconds[3], BinaryTensSeconds[4] );
      lcd.print(textbuffer);
      sprintf(textbuffer, "%1d%1d%1d%1d S", BinarySeconds[1], BinarySeconds[2], BinarySeconds[3], BinarySeconds[4] );
      lcd.print(textbuffer);

   


      if (Seconds <= SECONDS_CLOCK_HELP)  // show help: weighting
      {
      #ifdef FEATURE_CLOCK_SOME_SECONDS
        lcd.setCursor(9, 0); lcd.print(" 421 8421");
      #endif
      } 
      else
      {
       lcd.setCursor(9, 0); lcd.print("         ");
      }
      } 
      else
        // horisontal 5-bit binary
    {
      // convert to binary:
      decToBinary(Hour, BinaryHour);
      decToBinary(Minute, BinaryMinute);
      decToBinary(Seconds, BinarySeconds);
  
      lcd.setCursor(13, 1); sprintf(textbuffer, "%1d%1d%1d%1d%1d H", BinaryHour[0], BinaryHour[1], BinaryHour[2], BinaryHour[3], BinaryHour[4]);
      lcd.print(textbuffer);
  
      lcd.setCursor(13, 2); sprintf(textbuffer, "%1d%1d%1d%1d%1d M", BinaryMinute[0], BinaryMinute[1], BinaryMinute[2], BinaryMinute[3], BinaryMinute[4] );
      lcd.print(textbuffer);
  
      lcd.setCursor(13, 3); sprintf(textbuffer, "%1d%1d%1d%1d%1d S", BinarySeconds[0], BinarySeconds[1], BinarySeconds[2], BinarySeconds[3], BinarySeconds[4] );
      lcd.print(textbuffer);
  
      lcd.setCursor(0, 0); lcd.print("Binary");
  
      if (Seconds <= SECONDS_CLOCK_HELP)  // show help: weighting
      {
      #ifdef FEATURE_CLOCK_SOME_SECONDS
        lcd.setCursor(13, 0); lcd.print(" 8421");
      #endif
      } 
      else
      {
        lcd.setCursor(13, 0); lcd.print("     ");
      }
     }
  
        // Common for all modes:
      #ifdef FEATURE_CLOCK_SOME_SECONDS
          if (Seconds <= SECONDS_CLOCK_HELP)  // show time in normal numbers
          {
          sprintf(textbuffer, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
          } 
          else
          {
          sprintf(textbuffer, "        ");
          }
          lcd.setCursor(0, 3); // last line *********
          lcd.print(textbuffer);
        #endif
 }

// Menu item ////////////////////////////////////////////
void Bar(void) {
          char textbuffer[9];
          // get local time

          local = now() + UTCoffset * 60;
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
            lcd.write(255); // fills square
            if (i == 2 | i == 5 | i == 8) lcd.write(254); // empty
          }

          // could have used |, ||, |||, |||| for intermediate symbols by creating new characters
          // like here https://forum.arduino.cc/index.php?topic=180678.0
          // but easier to use something standard

          //  if (Minute/12 == 1) {lcd.write(165);}
          //  else if (Minute/12 == 2) {lcd.print('"');}
          //  else if (Minute/12 == 3) {lcd.write(208);}
          //  else if (Minute/12 == 4) {lcd.write(219);}

          //  lcd.print(" ");lcd.print(Hour);

          lcd.setCursor(18, 0); lcd.print("1h");

          lcd.setCursor(0, 1);
          imax = Minute / 5;
          if (Minute == 0) lcd.print("                ");
          for (int i = 0; i < imax; i++) {
            lcd.write(255); // fills square
            if (i == 2 | i == 5 | i == 8) lcd.write(254); // empty
          }
          if (Minute % 5 == 1) {
            lcd.write(165);
          }
          else if (Minute % 5 == 2) {
            lcd.print('"');
          }
          else if (Minute % 5 == 3) {
            lcd.write(208);
          }
          else if (Minute % 5 == 4) {
            lcd.write(219);
          }
          // lcd.print(" ");lcd.print(Minute);
          lcd.setCursor(18, 1); lcd.print("5m");


          // seconds in 12 characters, with a break every 3 characters
          lcd.setCursor(0, 2);
          imax = Seconds / 5;
          if (Seconds == 0) lcd.print("                ");
          for (int i = 0; i < imax; i++) {
            lcd.write(255); // fills square
            if (i == 2 | i == 5 | i == 8) lcd.write(254); // empty
          }
          if (Seconds % 5 == 1) {
            lcd.write(165);
          }
          else if (Seconds % 5 == 2) {
            lcd.print('"');
          }
          else if (Seconds % 5 == 3) {
            lcd.write(208);
          }  //("%");}
          else if (Seconds % 5 == 4) {
            lcd.write(219);
          }  //("#");}
          lcd.setCursor(18, 2); lcd.print("5s");
 //        lcd.setCursor(18, 3); lcd.print("  ");

          lcd.setCursor(18, 3);
          if (Hour > 12) lcd.print("PM");
          else lcd.print("AM");

          #ifdef FEATURE_CLOCK_SOME_SECONDS
            lcd.setCursor(8, 3);
            if (Seconds <= SECONDS_CLOCK_HELP)  // show time in normal numbers
            {
          //    lcd.print("Bar");
            sprintf(textbuffer, "%02d%c%02d%c%02d", Hour%12, HOUR_SEP, Minute, MIN_SEP, Seconds);

            lcd.print(textbuffer);

            }
            else
            {
              lcd.print("          ");
            }
          #endif
        }

// Menu item ////////////////////////////////////////////
void MengenLehrUhr(void) {
//
// Set theory clock of https://en.wikipedia.org/wiki/Mengenlehreuhr in Berlin
        int imax;
        //  lcd.clear(); // makes it blink

        // get local time
        local = now() + UTCoffset * 60;
        Hour = hour(local);
        Minute = minute(local);
        Seconds = second(local);

        lcd.setCursor(0, 0);
        // top line has 5 hour resolution
        if (Hour > 4)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");
        if (Hour > 9)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");

        if (Hour > 14)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");
        if (Hour > 19)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");

        lcd.setCursor(18, 0); lcd.print("5h");

        // second line shows remainder and has 1 hour resolution

        lcd.setCursor(0, 1);
        imax = Hour % 5;
        if (imax > 0)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");
        if (imax > 1)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");
        if (imax > 2)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");
        if (imax > 3)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
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
          lcd.write(255);
          if (ii == 2 || ii == 5 || ii == 8) lcd.print(" ");
        }

        lcd.setCursor(18, 2); lcd.print("5m");

        // fourth line shows remainder and has 1 minute resolution
        lcd.setCursor(0, 3);

        imax = Minute % 5;
        if (imax > 0)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");

        if (imax > 1)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");

        if (imax > 2)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");

        if (imax > 3)
        {
          lcd.print("-"); lcd.write(255); lcd.print("-"); lcd.print(" ");// fills square
        }
        else lcd.print("    ");

        lcd.setCursor(18, 3); lcd.print("1m");
      }

// Menu item ////////////////////////////////////////////    
void LinearUhr(void) {
// Linear Clock, https://de.wikipedia.org/wiki/Linear-Uhr in Kassel
     
        int imax;
        int ii;
        
        // get local time
        local = now() + UTCoffset * 60;
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
          lcd.write(255);
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
          lcd.write(255);
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
          lcd.write(255);
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
          lcd.write(255);
          if (ii == 4) lcd.print(" ");
        }
        lcd.setCursor(18, 3); lcd.print("1m");
      }


// Menu item //////////////////////////////////////////////////////////////////////////////////////////
void InternalTime() {     // UTC, Unix time, J2000, etc
      char textbuffer[20];

      lcd.setCursor(0, 0); // top line *********
      if (gps.time.isValid()) {

        float jd = now() / 86400.0; // cdn(now()); // now/86400, i.e. no of days since 1970
        float j2000 = jd - 10957.5; // 1- line
        lcd.print("j2k ");
        lcd.print(j2000);
        
        lcd.setCursor(12, 0);
        sprintf(textbuffer, "%02d%c%02d%c%02d UTC ", hourGPS, HOUR_SEP, minuteGPS, MIN_SEP, secondGPS);
        lcd.print(textbuffer);
                    
        lcd.setCursor(0, 1);
        lcd.print("jd1970 ");
        lcd.print(jd);

        // utc = now(); // UNIX time, seconds ref to 1970
        lcd.setCursor(0, 2);
        lcd.print("now   ");
        lcd.print(now());

        lcd.setCursor(0, 3);
        lcd.print("local ");
        local = now() + UTCoffset * 60;
        lcd.print(local);
      }
    }

// Menu item ////////////////////////////////////////////
void code_Status(void) {
    lcd.setCursor(0, 0); lcd.print("* LA3ZA GPS clock *");
    lcd.setCursor(0, 1); lcd.print("Ver "); lcd.print(CODE_VERSION);
    lcd.setCursor(0, 2); lcd.print("GPS "); lcd.print(GPSBaud); //lcd.print(" bps");
    lcd.setCursor(0, 3); lcd.print(tcr -> abbrev);lcd.print(" "); lcd.print(UTCoffset);  //timezone name and offset (min)   
  }

// Menu item //////////////////////////////////////////////////////////////////////////////////////////
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

           
      
      lcd.setCursor(0, 2);
      if ((now() / 4) % 3 == 0) { // change every 4 seconds

        //  decimal degrees
        lcd.setCursor(0, 2);
        textbuf = String(abs(latitude), 4);
        lcd.print(textbuf); lcd.write(DegreeSymbol); 
        if (latitude < 0) lcd.print(" S   "); 
        else lcd.print(" N   "); 

        lcd.setCursor(0, 3);
        textbuf = String(abs(lon), 4);
        int strLength = textbuf.length();      
        lcd.print(textbuf);lcd.write(DegreeSymbol);
        if (lon < 0) lcd.print(" W    "); 
        else lcd.print(" E    "); 
      }
      else if ((now() / 4) % 3 == 1) { 
        
        // degrees, minutes, seconds
        lcd.setCursor(0, 2);
        float mins;
        textbuf = String((int)abs(latitude)); 
        lcd.print(textbuf); lcd.write(DegreeSymbol); 
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
        lcd.write(DegreeSymbol); 
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
        lcd.print(textbuf); lcd.write(DegreeSymbol); 
        mins = abs(60 * (latitude - (int)latitude));
        textbuf = String(abs(mins), 2); 
        lcd.print(textbuf); 
        if (latitude < 0) lcd.print("' S "); 
        else lcd.print("' N "); 

        lcd.setCursor(0, 3);
        textbuf = String(int(abs(lon)));
        lcd.print(textbuf);
        lcd.write(DegreeSymbol); 
        mins = abs(60 * (lon - (int)lon)); 
        textbuf = String(abs(mins), 2);  // double abs() to avoid negative number for x.00 degrees
        lcd.print(textbuf); 
        if (lon < 0) lcd.print("' W  "); 
        else lcd.print("' E  "); 
      }    
    }
     // enough space on display for 2469 m
      lcd.setCursor(14, 2);
      printFixedWidth(lcd, (int)round(alt), 4, ' '); lcd.print(" m");
      
    if (gps.satellites.isValid()) {
      noSats = gps.satellites.value();
      if (noSats < 10) lcd.setCursor(14, 3);
      else lcd.setCursor(13, 3); lcd.print(noSats); lcd.print(" Sats");
    }
  }





// Menu items //////////////////////////////////////////////////////////////////////////////////

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
        locator_to_latlong(location[iii], lati, longi);// position of beacon
        km = distance(lati, longi, latitude, lon);    // distance beacon - GPS
        printFixedWidth(lcd, (int)float(km),6);
        lcd.print("km");
      }
    }
  }


// Menu item //////////////////////////////////////////////////////////////////////////////////
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

// Menu item //////////////////////////////////////////////////////////////////////////////////////////
void HexOctalClock(
  int val   // 0 - hex, 1- octal
  )
{
      char textbuf[21];
  //  get local time
      local = now() + UTCoffset * 60;
      Hour = hour(local);
      Minute = minute(local);
      Seconds = second(local);

      lcd.setCursor(0, 0);
      if (val == 0) lcd.print("Hex                 ");
      else          lcd.print("Octal               ");

      lcd.setCursor(7, 1);
      if (val == 0) sprintf(textbuf, "%02X%c%02X%c%02X",Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
      else          sprintf(textbuf, "%02o%c%02o%c%02o",Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
      lcd.print(textbuf);

      #ifdef FEATURE_CLOCK_SOME_SECONDS
            lcd.setCursor(7, 3);
            if (Seconds <= SECONDS_CLOCK_HELP)  // show time in normal numbers
            {
              sprintf(textbuf, "%02d%c%02d%c%02d", Hour, HOUR_SEP, Minute, MIN_SEP, Seconds);
              lcd.print(textbuf);
            }
            else
            {
              lcd.print("         ");
            }
       #endif
       lcd.setCursor(18, 3); lcd.print("  ");
}

  
 

////////////////////////////////////////////////////////////////////////////////////////////////////
// End of functions for Menu system ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/// THE END ///
