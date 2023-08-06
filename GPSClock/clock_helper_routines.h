////////////////////////////////////////////////////////////////////////////////////////////
/* Collection of helper functions //////////////////////////////////////////////////////////

GetNextRiseSet
MoonPhase
MoonPhaseAccurate
MoonWaxWane
MoonSymbol
UpdateMoonPosition

AnalogButtonRead

Maidenhead
LocatorToLatLong
Distance
DecToBinary

PrintFixedWidth
LcdDate
LcdUTCTimeLocator
LcdShortDayDateTimeLocal
LcdSolarRiseSet

ComputeEasterDate
JulianToGregorian

MathPlus
MathMinus
MathMultiply
MathDivide

printDouble
LcdMorse

LCDPlanetData

LCDChemicalElement
LCDChemicalGroupPeriod
LCDChemicalElementName

readIntFromEEPROM
updateIntIntoEEPROM
resetFunc
InitScreenSelect
RotarySecondarySetup
RotarySetup

///////////////////////////////////////////////////////////////////////////////////////////
*/

void GetNextRiseSet(
  short       *pRise,            // returned Moon Rise time
  double      *rAz,              // return Moon Rise Azimuth
  short       *pSet,             // returned Moon Set time
  double      *sAz,              // return Moon Set Azimuth
  int         *order              // 1 if rise is first, 2 if set is first
)
{
  short pLocal, pRise1, pSet1, pRise2, pSet2;
  double rAz1, sAz1, rAz2, sAz2;

  GetMoonRiseSetTimes(float(utcOffset) / 60.0, latitude, lon, &pRise1, &rAz1, &pSet1, &sAz1);

  *pRise = pRise1;
  *rAz = rAz1;
  *pSet = pSet1;
  *sAz = sAz1;

  local[timeZoneNumber] = now() + utcOffset * 60;

 // local = 1638052000; // 27.11.2021, ~23.30
  
  pLocal = 100 * hour(local[timeZoneNumber]) + minute(local[timeZoneNumber]);

#ifdef FEATURE_SERIAL_MOON
  //  Serial.print(F("zone "));Serial.println(zone);
  Serial.print(F("pRise, rAz  : ")); Serial.print(pRise1); Serial.print(F(", ")); Serial.println(rAz1);
  Serial.print(F("pSet, sAz   : ")); Serial.print(pSet1); Serial.print(F(", ")); Serial.println(sAz1);
  Serial.print(F("pLocal      : ")); Serial.println(pLocal);
#endif

  //  find rise/set times for next day also
  GetMoonRiseSetTimes(float(utcOffset) / 60.0 - 24.0, latitude, lon, (short*) &pRise2, (double*) &rAz2, (short*) &pSet2, (double*) &sAz2);

#ifdef FEATURE_SERIAL_MOON
  Serial.print(F("pRise2, rAz2: ")); Serial.print(pRise2); Serial.print(F(", ")); Serial.println(rAz2);
  Serial.print(F("pSet2, sAz2 : ")); Serial.print(pSet2); Serial.print(F(", ")); Serial.println(sAz2);
#endif

  if ((pLocal > pRise1) | (pLocal > pSet1)) {
    if (pRise1 < pSet1)
    {
      *pRise = pRise2;
      *rAz = rAz2;
      *order = 2;  // set before rise
      if (pLocal > pSet1)
      {
        *pSet = pSet2;
        *sAz  = sAz2;
        *order = 1;  // rise before set
      }
    }
    else // pRise1 >= pSet1
    {
      *pSet = pSet2;
      *sAz  = sAz2;
      *order = 1; // rise is first
      if (pLocal > pRise1)
      {
        *pRise = pRise2;
        *rAz  = rAz2;
        *order = 2;  // set is first
      }
    }

  }
  else
  {
    if (*pRise < *pSet) *order = 1; // 1 if rise is first, 2 if set is first
    else                *order = 2;
  }

  /*
      pRise = pSet = -2;  // the moon never sets
      pRise = pSet = -1;  // the moon never rises
      pRise = -1;               // no MoonRise and the moon sets
      pSet = -1;                // the moon rises and never sets
  */

#ifdef FEATURE_SERIAL_MOON
  Serial.print(F("order: ")); Serial.println(*order);
  Serial.print(F("pRise, rAz: ")); Serial.print(*pRise); Serial.print(F(", ")); Serial.println(*rAz);
  Serial.print(F("pSet, sAz : ")); Serial.print(*pSet);  Serial.print(F(", ")); Serial.println(*sAz);
#endif
}



////////////////////////////////////////////////////////////////////////////////////

// from https://community.facer.io/t/moon-phase-formula-updated/35691

// (((#DNOW#-583084344)/2551442802)%1) // DNOW in ms, UNIX time
#define REF_TIME 583084
#define CYCLELENGTH 2551443 // sec <=> 29.53059 days: only defined here

void MoonPhase(float& Phase, float& PercentPhase) {
//  const float moonMonth = 29.530588; // varies from 29.18 to about 29.93 days
  const float moonMonth = CYCLELENGTH / 86400.; // 29.53 days = average, varies from 29.18 to about 29.93 days
  
  long dif, moon;

  dif = (now() - REF_TIME); // Seconds since reference new moon
  moon = dif % CYCLELENGTH; // Seconds since last new moon

#ifdef FEATURE_SERIAL_MOON
  Serial.print(F("MoonPhase: now, dif, moon: "));
  Serial.println(now());
  Serial.println(dif);
  Serial.println(moon);
#endif

  Phase = abs(float(moon) / float(86400));               // in days

#ifdef FEATURE_SERIAL_MOON
  Serial.print(Phase); Serial.println(F(" days"));
#endif

  // The illumination approximates a sine wave through the cycle.
  // PercentPhase = 100 - 100*abs((Phase - 29.53/2)/(29.53/2)); // too big 39%, should be 31%
  // PercentPhase = 100*(sin((PI/2)*(1 - abs((Phase - moonMonth/2)/(moonMonth/2))))); // even bigger 57%
  // PercentPhase = 100*((2/pi)*asin(1 - abs((Phase - moonMonth/2)/(moonMonth/2)))) ;  //  too small 25%
  //http://www.dendroboard.com/forum/parts-construction/280865-arduino-moon-program.html

  PercentPhase = 50 * (1 - cos(2 * PI * Phase / moonMonth)); // in percent

#ifdef FEATURE_SERIAL_MOON
  Serial.print(PercentPhase); Serial.print(F("% "));
  Serial.print(Phase);        Serial.println(F(" days"));
#endif

}


// http://www.moongiant.com/moonphases/
// http://www.onlineconversion.com/unix_time.htm

//////////////////////////////////////////////////////////////////////////////////////////////////////
// More accurate formula?
/* Phase =

(((#DNOW#/2551442844-0.228535)
+0.00591997sin(#DNOW#/5023359217+3.1705094)
+0.017672776sin(#DNOW#/378923968-1.5388144)
-0.0038844429sin(#DNOW#/437435791+2.0017235)
-0.00041488sin(#DNOW#/138539900-1.236334))%1)

DNOW in ms, UNIX time

The result is a real number in the range 0 to 1
*/

void MoonPhaseAccurate(float& Phase, float& PercentPhase) {
//  const float moonMonth = 29.53059; // varies from 29.18 to about 29.93 days
  const float moonMonth = CYCLELENGTH / 86400.; // 29.53 days
  
  long dif, moon;
  time_t dnow;
  
  dnow = now(); // Unix time in sec
  dif = (dnow - REF_TIME);                      // Seconds since reference new moon
  dif = (dif % CYCLELENGTH) - 0.228535;         // seconds since last new moon

  // These three terms can max contribute 0.006+0.018+0.004 = 0.028 seconds, i.e. they are insignificant:
  dif = dif + 0.00591997*sin(dnow/5023359+3.1705094);   
  dif = dif + 0.017672776*sin(dnow/378924-1.5388144);   
  dif = dif - 0.0038844429* sin(dnow/437436+2.0017235); 
  
  moon = dif - 0.00041488*sin(dnow/138540-1.236334); // Seconds since last new moon
 
#ifdef FEATURE_SERIAL_MOON
  Serial.print(F("MoonPhaseAccurate: now, dif, moon: "));
  Serial.println(now());
  Serial.println(dif);
  Serial.println(moon);
#endif

  Phase = abs(float(moon) / float(86400));               // in days

#ifdef FEATURE_SERIAL_MOON
  Serial.print(Phase); Serial.println(F(" days"));
#endif

  PercentPhase = 50 * (1 - cos(2 * PI * Phase / moonMonth)); // in percent

#ifdef FEATURE_SERIAL_MOON
  Serial.print(PercentPhase); Serial.print(F("% "));
  Serial.print(Phase);        Serial.println(F(" days"));
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void MoonWaxWane(float Phase) {
  // lcd.print an arrow

  float CycleDays = CYCLELENGTH / 86400.;
  float delta; // = 1.0;

  delta = OPTION_DAYS_WITHOUT_MOON_ARROW/2;
  
  if (Phase <= delta)                                                       lcd.print(' '); // hardly visible
  else if ((Phase > delta) && (Phase < CycleDays / 2. - delta))                 lcd.write(DASHED_UP_ARROW); // Waxing moon
  else if ((Phase >= CycleDays / 2. - delta) && (Phase <= CycleDays / 2. + delta))  lcd.print(' '); // Full moon
  else if ((Phase > CycleDays / 2. + delta) && (Phase < CycleDays - delta))       lcd.write(DASHED_DOWN_ARROW); //  Waning moon
  else                                                                      lcd.print(' '); // hardly visible

}

///////////////////////////////////////////////////////////////////////////////
void MoonSymbol(float Phase) {
  // lcd.print an ( or ) symbol [

  float CycleDays = CYCLELENGTH / 86400.; // 29.53 days
  float delta; // 1.0
  
  delta = OPTION_DAYS_WITHOUT_MOON_SYMBOL/2.0;
  
  if (Phase <= delta)                                                       lcd.print(' '); // hardly visible
  else if ((Phase > delta) && (Phase < CycleDays / 2. - delta))                 lcd.print(')'); // Waxing moon: ny
  else if ((Phase >= CycleDays / 2. - delta) && (Phase <= CycleDays / 2. + delta))  lcd.print('o'); // Full moon
  else if ((Phase > CycleDays / 2. + delta) && (Phase < CycleDays - delta))       lcd.print('('); // Waning Moon: ne
  else                                                                      lcd.print(' '); // hardly visible
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateMoonPosition() {
  // from K3NG
//  String textbuf;

  double RA, Dec, topRA, topDec, LST, HA;


  // UTC time:
  moon2(yearGPS, monthGPS, dayGPS, (hourGPS + (minuteGPS / 60.0) + (secondGPS / 3600.0)), lon, latitude, &RA, &Dec, &topRA, &topDec, &LST, &HA, &moon_azimuth, &moon_elevation, &moon_dist);

#ifdef FEATURE_SERIAL_MOON
  Serial.print(F("moon2: "));
  Serial.print(F("RA, DEC: "));
  Serial.print(RA); Serial.print(F(", ")); Serial.println(Dec);
#endif
}



//------------------------------------------------------------------

byte AnalogButtonRead(byte button_number) {
  // K3NG keyer code
  // button numbers start with 0

  int analog_line_read = analogRead(analog_buttons_pin);
  // 10 k from Vcc to A0, then n x 1k to gnd, n=0...9;
  //if (analog_line_read < 500) { // any of 10 buttons will trigger

  if (analog_line_read < 131 & analog_line_read > 50) { // button 1
    return 1;
  }
  else if (analog_line_read < 51) { // button 0
    return 2;
  }
  else  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void Maidenhead(double lon, double latitude, char loc[7]) {
  int lonTrunc, latTrunc, lonFirst, latFirst, lonSec, latSec;

  // Locator calculation  
  // http://en.wikipedia.org/wiki/Maidenhead_Locator_System
  // https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square

  // Examples for debugging:
  //   BH52jn: lon = -14.9176000; latitude = -17.438000;
  //   FN31pr: lon = -72.727260;  latitude = 41.714775;
  //   JO59jw; lon = 10.750000;   latitude = 59.945556;
  //   JJ00aa: lon = 0;          latitude = 0;

  lon += 180.0;     // only positive: 0...360
  latitude += 90.0; // only positive: 0...180
  lonTrunc = ((int)(lon * 20 )) / 20.0;
  latTrunc = ((int)(latitude * 10 )) / 10.0;

  lonFirst = lonTrunc / 20;
  latFirst = latTrunc / 10;

  lonSec = (lonTrunc % 20) / 2;
  latSec = (latTrunc % 10) / 1;
  
  loc[0] = 'A' + lonFirst;                                  // 1 field = 20 deg
  loc[1] = 'A' + latFirst;                                  // 1 field = 10 deg
  
  loc[2] = '0' + lonSec;                                    // 1 square = 2 deg
  loc[3] = '0' + latSec;                                    // 1 square = 1 deg
  
  loc[4] = 'a' + (lon      - lonFirst*20 - lonSec*2) * 12;  // 1 subsquare = 5' = 1/12 deg
  loc[5] = 'a' + (latitude - latFirst*10 - latSec  ) * 24;  // 1 subsquare = 2.5' = 1/24 deg
  
  loc[6] = '\0';
}

//------------------------------------------------------------------

void LocatorToLatLong(char loc[7], double &latitude, double &longitude) {
  /*
    convert locator to latitude, longitude
    based on pyhamtools.locator http://pyhamtools.readthedocs.io/en/stable/reference.html#module-pyhamtools.locator
    locator  must have 6 digits
  */
  int i;
  for (i = 0; i < 6; i += 1) {
    loc[i] = toUpperCase(loc[i]);
  }

  longitude = ((int)loc[0] - (int)('A')) * 20.0 - 180.0;
  latitude = ((int)loc[1] - (int)('A')) * 10.0 - 90.0;

  longitude += ((int)loc[2] - (int)('0')) * 2.0;
  latitude += ((int)loc[3] - (int)('0'));

  longitude += ((int)loc[4] - (int)('A')) * (2.0 / 24.0);
  latitude  += ((int)loc[5] - (int)('A')) * (1.0 / 24.0);

  // move to center of subsquare
  longitude += 1.0 / 24.0;
  latitude += 0.5 / 24.0;
}

//////////////////////////////////////////////////////////////////////////////////////

int Distance(double lat1, double long1, double lat2, double long2) {
  /*
      Calculate distance between two positions on earth,
      angles in degrees, result truncated to nearest km
      based on pyhamtools.locator http://pyhamtools.readthedocs.io/en/stable/reference.html#module-pyhamtools.locator
  */
  float d_lat, d_long, r_lat1, r_lat2, r_long1, r_long2, a, c;
  int km;
  float deg_per_rad, R;

  deg_per_rad = 57.29578; // degrees per radian

  R = 6371; // earth radius in km. Earth is assumed a perfect sphere

  r_lat1  = lat1 / deg_per_rad;
  r_long1 = long1 / deg_per_rad;
  r_lat2  = lat2 / deg_per_rad;
  r_long2 = long2 / deg_per_rad;

  d_lat  = r_lat2 - r_lat1;
  d_long = r_long2 - r_long1;

  a = sin(d_lat / 2) * sin(d_lat / 2) + cos(r_lat1) * cos(r_lat2) * sin(d_long / 2) * sin(d_long / 2);
  c = 2 * atan2(sqrt(a), sqrt(1 - a));
  km = R * c + 0.5; //distance in km (+0.5 to make truncation to integer into a round operation)

  return km;
}



////////////////////////////////////////////////////////////////////////////////////////////////////

void DecToBinary(int n, int binaryNum[])
{
  //   https://www.geeksforgeeks.org/program-decimal-binary-conversion/
  // array to store binary number
  // LSB in position 3 (was 0)

  binaryNum[0] = 0;   binaryNum[1] = 0;   binaryNum[2] = 0;   binaryNum[3] = 0;  binaryNum[4] = 0; binaryNum[5] = 0;
  // counter for binary array
  int i = 5;

  while (n > 0) {

    // storing remainder in binary array
    binaryNum[i] = n % 2;
    n = n / 2;
    i--;
  }
}

//////////////////////////////////////////////////

void PrintFixedWidth(Print &out, int number, byte width, char filler = ' ') {
  int temp = number;
  //
  // call like this to print number to lcd: PrintFixedWidth(lcd, val, 3);
  // or for e.g. minutes PrintFixedWidth(lcd, minute, 2, '0')
  //
  // Default filler = ' ', can also be set to '0' e.g. for clock
  // If filler = ' ', it handles negative integers: width = 5 => '   -2'
  // but not if filler = '0': width = 5 => '000-2'
  //
  // https://forum.arduino.cc/t/print-lcd-text-justify-from-right/398351/5
  // https://forum.arduino.cc/t/u8glib-how-to-display-leading-zeroes/396694/3

  //do we need room for a minus?
  if (number < 0) {
    width--;
  }

  //see how wide the number is
  if (temp == 0) {
    width--;
  }
  else
  {
    while (temp && width) {
      temp /= 10;
      width--;
    }
  }

  //start by printing the rest of the width with filler symbol
  while (width) {
    out.print(filler);
    width--;
  }

  out.print(number); // finally print signed number
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LcdUTCTimeLocator(int lineno, int col=0)
// function that displays the following kind of info on lcd row "lineno" and start column "col" (only 0 or 1)
//  "22:30:46 UTC  JO59fu"
{

#ifdef FEATURE_PC_SERIAL_GPS_IN
        hourGPS = hour(now());
        minuteGPS = minute(now());
        secondGPS = second(now());
#endif


//  if (gps.time.isValid()) {
    lcd.setCursor(min(max(col,0),1), lineno);
    sprintf(textBuffer, "%02d%c%02d%c%02d UTC ", hourGPS, dateTimeFormat[dateFormat].hourSep, minuteGPS, dateTimeFormat[dateFormat].minSep, secondGPS);
    lcd.print(textBuffer);
//  }

  if (gps.satellites.isValid()) {

#ifndef DEBUG_MANUAL_POSITION
    latitude = gps.location.lat();
    lon = gps.location.lng();
#else
    latitude = latitude_manual;
    lon      = longitude_manual;
#endif

//    char locator[7];
    Maidenhead(lon, latitude, textBuffer);
    lcd.setCursor(14, lineno);
    lcd.print(textBuffer);
  }
}

//------------------------------------------------------------------

void LcdDate(int Day, int Month, int Year=0) // print date, either day-month or day-month-year according to specified format
{
  if (dateTimeFormat[dateFormat].dateOrder == 'B')
    {
      if (Year !=0) 
      { 
        PrintFixedWidth(lcd, Year, 4); lcd.print(dateTimeFormat[dateFormat].dateSep);
      }
      PrintFixedWidth(lcd, Month, 2, '0'); lcd.print(dateTimeFormat[dateFormat].dateSep);
      PrintFixedWidth(lcd, Day, 2, '0');
    }
    else if (dateTimeFormat[dateFormat].dateOrder == 'M')
    {
      PrintFixedWidth(lcd, Month, 2, '0'); lcd.print(dateTimeFormat[dateFormat].dateSep);
      PrintFixedWidth(lcd, Day, 2, '0'); 
      if (Year !=0) 
      {
        lcd.print(dateTimeFormat[dateFormat].dateSep);PrintFixedWidth(lcd, Year, 4);
      }
    }
    else
    {
      PrintFixedWidth(lcd, Day, 2, '0'); lcd.print(dateTimeFormat[dateFormat].dateSep);
      PrintFixedWidth(lcd, Month, 2, '0'); 
      if (Year !=0) 
      {
        lcd.print(dateTimeFormat[dateFormat].dateSep);PrintFixedWidth(lcd, Year, 4);
      }
    }
  
}


//------------------------------------------------------------------

void LcdShortDayDateTimeLocal(int lineno = 0, int moveLeft = 0) {
  // function that displays the following kind of info on lcd row "lineno"
  //  "Wed 20.10     22:30:46" - date separator in fixed location, even if date is ' 9.8'
  // get local time
  local[timeZoneNumber] = now() + utcOffset * 60;
  Hour = hour(local[timeZoneNumber]);
  Minute = minute(local[timeZoneNumber]);
  Seconds = second(local[timeZoneNumber]);
  
  // local date
  Day = day(local[timeZoneNumber]);
  Month = month(local[timeZoneNumber]);
  Year = year(local[timeZoneNumber]);
    
  lcd.setCursor(0, lineno);
  if (dayGPS != 0)
  {
     if (languageNumber >=0)
      {      
          nativeDayLong(local[timeZoneNumber]);
          // 17.05.2023:
          if ((strcmp(languages[languageNumber],"es") == 0) || (strcmp(languages[languageNumber],"de") == 0) )  
               sprintf(textBuffer,"%2.2s",today);  // 2 letters for day name in Spanish, German
          else sprintf(textBuffer,"%3.3s",today);  // else 3 letters
          lcd.print(textBuffer);
      }
 else
 {
         #ifdef FEATURE_DAY_PER_SECOND
    //      fake the day -- for testing only
              sprintf(textBuffer, "%3s", dayShortStr( 1+(local[timeZoneNumber]/2)%7 )); // change every two seconds
         #else
              sprintf(textBuffer, "%3s", dayShortStr(weekday(local[timeZoneNumber])));
         #endif     
         #ifdef FEATURE_SERIAL_MENU
              Serial.println("LcdShortDayDateTimeLocal: ");
              Serial.println("  weekday(local), today");
              Serial.println(weekday(local[timeZoneNumber]));
              Serial.println(textBuffer);
         #endif
          
         lcd.print(textBuffer); lcd.print(" ");
 }
 //     #endif
     
        lcd.setCursor(4, lineno);
    
        if ((dateTimeFormat[dateFormat].dateOrder == 'M') | (dateTimeFormat[dateFormat].dateOrder == 'B'))
        {
// modified so month takes up a fixed space without a leading zero:
//          lcd.print(static_cast<int>(Month)); lcd.print(dateTimeFormat[dateFormat].dateSep);
            PrintFixedWidth(lcd, Month, 2,' '); lcd.print(dateTimeFormat[dateFormat].dateSep);
            lcd.print(static_cast<int>(Day));
        }
        else
        {
// modified so day takes up a fixed space without a leading zero:
//        lcd.print(static_cast<int>(Day)); lcd.print(dateTimeFormat[dateFormat].dateSep);
          PrintFixedWidth(lcd, Day, 2,' '); lcd.print(dateTimeFormat[dateFormat].dateSep);
          lcd.print(static_cast<int>(Month));
        }
      }
      lcd.print(F("    ")); // in order to erase remnants of long string as the month changes
      lcd.setCursor(11 - moveLeft, lineno);
      sprintf(textBuffer, " %02d%c%02d%c%02d", Hour, dateTimeFormat[dateFormat].hourSep, Minute, dateTimeFormat[dateFormat].minSep, Seconds); // corrected 18.10.2021
      lcd.print(textBuffer);
    }

/////////////////////////////////////////////////////////////////////////////////////////

void LcdSolarRiseSet(
  int lineno,                       // lcd line no 0, 1, 2, 3
  char RiseSetDefinition = ' ',     // default - Actual, C - Civil, N - Nautical, A - Astronomical,  O - nOon info, Z - aZ, el info
  int  ScreenMode = ScreenLocalSun  // One of ScreenLocalSun, ScreenLocalSunSimpler, ScreenLocalSunMoon, ScreenLocalSunAzEl
)
{
  // Horizon for solar rise/set: Actual (0 deg), Civil (-6 deg), Nautical (-12 deg), Astronomical (-18 deg)

  lcd.setCursor(0, lineno);

  // create a Sunrise object
  Sunrise mySunrise(latitude, lon, float(utcOffset) / 60.);

  byte h, m;
  int hNoon, mNoon;
  int t; // t= minutes past midnight of sunrise (6 am would be 360)
  cTime c_time;
  cLocation c_loc;
  cSunCoordinates c_sposn;

  // https://www.timeanddate.com/astronomy/different-types-twilight.html
  if (RiseSetDefinition == 'A')     // astronomical: -18 deg
  // "During astronomical twilight, most celestial objects can be observed in the sky. However, the atmosphere still scatters and 
  // refracts a small amount of sunlight, and that may make it difficult for astronomers to view the faintest objects."
        mySunrise.Astronomical();
        
  else if (RiseSetDefinition == 'N') // Nautical:     -12 deg
  // "nautical twilight, dates back to the time when sailors used the stars to navigate the seas. 
  // During this time, most stars can be easily seen with naked eyes, and the horizon is usually also visible in clear weather conditions."
        mySunrise.Nautical();
        
  else if (RiseSetDefinition == 'C') // Civil:        - 6 deg 
  // "enough natural sunlight during this period that artificial light may not be required to carry out outdoor activities."
        mySunrise.Civil();
        
  else  mySunrise.Actual();           // Actual          0 deg


  // First: print sun rise time
  t = mySunrise.Rise(monthGPS, dayGPS); // Sun rise hour minute

  if (t >= 0) {             // if not satisfied, then e.g. for 'N' then sun never dips below 18 deg at night, as in mid summer in Oslo
  
    h = mySunrise.Hour();
    m = mySunrise.Minute();

    if (ScreenMode == ScreenLocalSunSimpler | ScreenMode == ScreenLocalSunAzEl) lcd.print(" "); // to line up rise time with date on line above
    
    if (RiseSetDefinition == ' ') // Actual
    {
      lcd.print(F("  ")); lcd.write(UP_ARROW);
    }
    else if (RiseSetDefinition == 'C')  // Civil
    {
      lcd.print(F("  ")); lcd.write(DASHED_UP_ARROW);
    }
    else lcd.print(F("   "));              // Nautical

    if (lineno==1) 
    {
        lcd.setCursor(0, lineno);
        lcd.print(F("S "));
    }

    if (RiseSetDefinition == ' ' |RiseSetDefinition == 'C'|RiseSetDefinition == 'N'|RiseSetDefinition == 'A')
    {
    
    if (ScreenMode == ScreenLocalSunSimpler | ScreenMode == ScreenLocalSunAzEl) lcd.setCursor(4, lineno);
    else lcd.setCursor(3, lineno);
    
    PrintFixedWidth(lcd, h, 2);
    lcd.print(dateTimeFormat[dateFormat].hourSep);
    PrintFixedWidth(lcd, m, 2, '0');
    }
  }
  
  // Second: print sun set time

  t = mySunrise.Set(monthGPS, dayGPS); // Sun set time

  lcd.setCursor(9, lineno);
  if (ScreenMode == ScreenLocalSunSimpler| ScreenMode == ScreenLocalSunAzEl) lcd.print(F("  "));
  if (RiseSetDefinition == ' ')       lcd.write(DOWN_ARROW);
  else if (RiseSetDefinition == 'C')  lcd.write(DASHED_DOWN_ARROW);
  else                                lcd.print(" ");

  if (t >= 0) {   
    h = mySunrise.Hour();
    m = mySunrise.Minute();

    if (RiseSetDefinition == ' ' |RiseSetDefinition == 'C'|RiseSetDefinition == 'N'|RiseSetDefinition == 'A')
    {
      // improved format 18.06.2022
      PrintFixedWidth(lcd, h, 2);
      lcd.print(dateTimeFormat[dateFormat].hourSep);
      PrintFixedWidth(lcd, m, 2, '0');    
      
//      lcd.print(h, DEC);
//      lcd.print(dateTimeFormat[dateFormat].hourSep);
//      if (m < 10) lcd.print("0");
//      lcd.print(m, DEC);
//      
    }
  }

// 18.06.2022: the following if {} moved out of if {} above, in order to show e.g. 'N' even around midsummer
  if (ScreenMode == ScreenLocalSunSimpler) 
  {
        lcd.setCursor(18, lineno);
        lcd.print(" ");
        lcd.print(RiseSetDefinition); // show C, N, A to the very right
  }

SolarElevation:

  /////// Solar elevation //////////////////

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

   if (RiseSetDefinition == 'Z') // print current aZimuth, elevation
      {
        lcd.setCursor(0, lineno);
        lcd.print(F("nowEl "));              // added "now" 18.6.2023
        PrintFixedWidth(lcd, (int)float(sun_elevation), 3);
        lcd.write(DEGREE);
        lcd.setCursor(11, lineno);
        lcd.print(F("Az "));
        PrintFixedWidth(lcd, (int)float(sun_azimuth), 3);
        lcd.write(DEGREE);
        lcd.print(F("  "));         
      }

  ///// Solar noon

  Sunrise my2Sunrise(latitude, lon, float(utcOffset) / 60.);
  t = my2Sunrise.Noon(monthGPS, dayGPS);
  if (t >= 0) {  
    hNoon = my2Sunrise.Hour();
    mNoon = my2Sunrise.Minute();
  }

  // find max solar elevation, i.e. at local noon
  
  c_time.dHours = hNoon - utcOffset / 60.;
  c_time.dMinutes = mNoon;
  c_time.dSeconds = 0.0;

  sunpos(c_time, c_loc, &c_sposn);

  // Convert Zenith angle to elevation
  float sun_elevationNoon = 90. - c_sposn.dZenithAngle;
  float sun_azimuthNoon = c_sposn.dAzimuth;

 // Right margin text 
  //    ' ': Solar elevation right now
  //    'C': Time for local noon
  //    'N': Solar elevation at local noon
  //    'A': -

  if (ScreenMode == ScreenLocalSun | ScreenMode == ScreenLocalSunMoon)
  {
  // position rightmargin info here:
    lcd.setCursor(16, lineno);
 
    if (RiseSetDefinition == ' ')
    { 
      PrintFixedWidth(lcd, (int)float(sun_elevation), 3);
      lcd.write(DEGREE);
    }
    else if (RiseSetDefinition == 'C')
    {
      if (t >= 0) {
        if (hNoon < 10) lcd.setCursor(16, 2); // added 4.7.2016 to deal with summer far North
        lcd.print(hNoon, DEC);
        //          lcd.print(dateTimeFormat[dateFormat].hourSep);  
        if (mNoon < 10) lcd.print("0");
        lcd.print(mNoon, DEC);
      }
    }
    else if (RiseSetDefinition == 'N')
    {
      // Noon data:
  
      PrintFixedWidth(lcd, (int)float(sun_elevationNoon), 3);
      lcd.write(DEGREE);
    }  
    
}      // if (ScreenMode == ...)


        if (RiseSetDefinition == 'O') // print sun's data at nOon
        {
          lcd.setCursor(0, lineno);
          lcd.print(F("maxEl "));         // added "max" 18.6.2023
          PrintFixedWidth(lcd, (int)float(sun_elevationNoon), 3);
          lcd.write(DEGREE);              // added 27.04.2022
          lcd.print(" ");   
          lcd.setCursor(12, lineno);
          PrintFixedWidth(lcd, hNoon, 2);
          lcd.print(dateTimeFormat[dateFormat].hourSep); 
          PrintFixedWidth(lcd, mNoon, 2,'0');         
          lcd.print(F("  "));       
        }
}

////////////////////////////////////////////////////////////////////////////////
void ComputeEasterDate( // find date of Easter Sunday
  int yr,         // input value for year
  int K, int E,   // see below
  int *PaschalFullMoon, // date in March for Paschal Full Moon (32 <=> 1 April and so on)
  int *EasterDate,      // date in March, April, May. Range: 22. March-25. April  
                        // Range: 4. April-8. May for Julian calendar in Gregorian dates
  int *EasterMonth      // 3, 4, or 5
)
/* Parameters K, E:
 *  Julian calendar: 
 *  K=-3 E=-1 
 *  
 *  Gregorian calendar: 
 *  1583-1693 K= l E=-8 
 *  1700-1799 K= 0 E=-9 
 *  1800-1899 K=-l E=-9 
 *  1900-2099 K=-2 E=-10 
 *  2100-2199 K=-3 E=-10
 */ 
{
  /*
  Werner Bergmann, Easter and the Calendar: The Mathematics of Determining 
  a Formula for the Easter Festival to Medieval Computing, 
  Journal for General Philosophy of Science / 
  Zeitschrift für allgemeine Wissenschaftstheorie , 1991
  Algorithm from pages 28-29, Bergmann

  Gregorian: Dates are given in Gregorian dates, i.e. 13 days before Julian dates at present
  
  Agrees with Julian and Gregorian dates here: https://webspace.science.uu.nl/~gent0113/easter/easter_text4c.htm
  but not with Julian dates here: https://en.wikipedia.org/wiki/List_of_dates_for_Easter 
  as 13 needs to be added to date in order to give Julian Easter in Gregorian dates
  */
  
  int x,y,z, n, concurrent, epact, ES; 
 
  // I. Concurrent
  x = yr-8;
  y = floor(x/4);         // leap year cycle
  z = x%4;
  concurrent = (x+y+K)%7;
  
  // II. Epact
  y = yr%19;            // 19 year periodicity of moon
  epact = (y*11 + E)%30;
  
  // III. Paschal Full Moon 
  if (epact <= 14)  *PaschalFullMoon = 21+14-epact;
  else              *PaschalFullMoon = 21+44-epact;
  
  // IV. Easter Sunday
  // 21 + (8 - Cone.) + n * 7 > paschal full moon
  
  n = ceil(float(*PaschalFullMoon-21-(8-concurrent))/7 + 0.001); // added 0.001 because > is the condition, not >=. Check year 2001
  *EasterDate = 21+(8-concurrent)+n*7;
  if (*EasterDate <= 31)
  {
    *EasterMonth = 3;
    *EasterDate = *EasterDate;
  }
  else if (*EasterDate <=61)
  {
    *EasterMonth = 4;
    *EasterDate = *EasterDate-31;
  }
  else
  {
    *EasterMonth = 5;
    *EasterDate = *EasterDate-61;
  }

//  Serial.print(F("concurrent, epact, PaschalFullMoon, n "));Serial.print(concurrent); Serial.print(" "); 
//  Serial.print(epact);Serial.print(" "); Serial.print(*PaschalFullMoon);Serial.print(" "); Serial.println(n);
  
 }

 ///////////////////////////////////////////////////////////////////////////////////////////////

 void JulianToGregorian(int *Date, int *Month)
  {
  // Add 13 days to get the dates in Gregorian notation (valid this century++):
  // only valid for dates in March and April

    *Date = *Date + 13;
  
  if (*Date > 31 & *Month == 3)
  {
    *Date = *Date - 31;
    *Month = *Month + 1;
  }
  else if (*Date > 30 & *Month == 4)
  {
    *Date = *Date - 30;
    *Month = *Month + 1; 
  }
 } 

 ///////////////////////////////////////////////////////////////////////////////////////////////

// void MathPlusMinus(int Term0,       // input number 
//                      int *Term1,      // output factor one
//                      int *Term2,      // output factor two 
//                      int OptionMath // 0, 1, ...
// )
// {
//  // random(min,max)
//  // min: lower bound of the random value, inclusive (optional).
//  // max: upper bound of the random value, exclusive.
//
//  if (OptionMath == 1) // +/- with equal probabilities
//    {
//        *Term1 = 0;  // avoid Term1 = 0
//        while (*Term1 == 0 | *Term1 == Term0)  *Term1 = random(max(0,Term0-9), Term0+10); // limit  term to +/-1...9: 
//    }
//    else if (OptionMath == 0) // +
//    {
//        // hour = 0 => 0+0
//        *Term1 = random(max(0,Term0-9), Term0+1);   // limit term1 to 0...term0 for OptionMath=0
//    }
//  
//  *Term2 = Term0 - *Term1;
// }

///////////////////////////////////////////////////////////////////////////////////////////////

 void MathPlus(int Term0,       // input number 
                      int *Term1,      // output factor one
                      int *Term2      // output factor two 
 )
 {
  #ifdef FEATURE_SERIAL_MATH
      Serial.print("*** MathPlus:                 + ");Serial.println(Term0);
  #endif
  // random(min,max)
  // min: lower bound of the random value, inclusive (optional).
  // max: upper bound of the random value, exclusive.

  // hour = 0 => 0+0: must allow zero
  *Term1 = random(max(0,Term0-9), Term0+1);   // limit Term1 to 1...Term0
  *Term2 = Term0 - *Term1;
  if (*Term2 > *Term1) // sort to get smallest last
    {
      int tmp = *Term1; *Term1 = *Term2; *Term2 = tmp;
    }  
 }

///////////////////////////////////////////////////////////////////////////////////////////////

 void MathMinus(int Term0,       // input number 
                      int *Term1,      // output factor one
                      int *Term2      // output factor two 
 )
 {
  #ifdef FEATURE_SERIAL_MATH
      Serial.print("*** MathMinus:                - ");Serial.println(Term0);
  #endif
  // random(min,max)
  // min: lower bound of the random value, inclusive (optional).
  // max: upper bound of the random value, exclusive.

  // avoid Term1=Term0, i.e. 0-0
  *Term1 = random(max(1,Term0+1), Term0+10); // limit  Term1 to 1 ... Term0+9 
  *Term2 = Term0 - *Term1;
        #ifdef FEATURE_SERIAL_MATH
          Serial.print("Term2, Term1 ");Serial.print(*Term1);Serial.print("  ");Serial.println(*Term2);
        #endif 
 }

 /////////////////////////////////////////////////////////////////////////////////////
 void MathMultiply( int Term0,       // input number 
                      int *Term1,      // output factor one
                      int *Term2      // output factor two 
 )
 {
  int i, j, k;
   // must have one more term than highest possible value of minute:
  int possible[] = {1, 2, 3, 4, 5, 7, 8, 11, 12, 13, 15, 16, 17, 18, 19, 20, 23,24,25,26,27,28, 29,30, 31, 37, 41, 43, 47, 53, 59, 61};
  int factors[15];

      #ifdef FEATURE_SERIAL_MATH
            Serial.print("*** MathMultiply:             * ");Serial.println(Term0);
      #endif
  
      if (Term0 !=0)
      {
        
        j = 0;
        for (i = 0; possible[i] <= Term0; i++)
         {
          
          if (Term0%possible[i] == 0)
          {
            factors[j] = possible[i];
            j = j+1; 
            #ifdef FEATURE_SERIAL_MATH
              Serial.print("i, possible[i] ");
              Serial.print(i);Serial.print("  ");
              Serial.println(possible[i]);
            #endif
          }
         }
    // random(min,max)
    // min: lower bound of the random value, inclusive (optional).
    // max: upper bound of the random value, exclusive.
  
    #ifdef FEATURE_SERIAL_MATH
      Serial.print("factors[i] ");Serial.print(factors[0]);Serial.print("  ");Serial.print(factors[1]);Serial.print("  ");
      Serial.print(factors[2]);Serial.print("  ");Serial.println(factors[3]);
    #endif 
    // choose randomly among possible factors stored in factors:
        k = random(0,j);
        *Term1 = factors[k]; 
      #ifdef FEATURE_SERIAL_MATH
        Serial.print("k, Term1 ");Serial.print(k);Serial.print("  ");Serial.println(*Term1);
      #endif 
      *Term2 = Term0 / *Term1; 
      if (*Term2 > *Term1) // sort to get smallest last
        {
          int tmp = *Term1; *Term1 = *Term2; *Term2 = tmp;
        }   
      }
      else //Term0 == 0
      {
        *Term1=0;
        *Term2=0;
      }  
 }

void MathDivide(int Term0,      // input number 
                int *Term1,     // output factor one: Term0 = Term1/Term2
                int *Term2      // output factor two 
)
{
  int factors[15];
  int y, n, j, k;

  #ifdef FEATURE_SERIAL_MATH
      Serial.print("*** MathDivide:               : ");Serial.println(Term0);
  #endif


  if (Term0 !=0)
      {
        j = 0;
        // find divisor > 0, < sqrt() and with only two digits in first factor:
        for (n = 1; (n <= sqrt(Term0) & Term0*n < 100); n++)
        {
          if (Term0%n == 0)
                {
                  factors[j] = n;
                  #ifdef FEATURE_SERIAL_MATH
                    Serial.print("j, factors[j] ");
                    Serial.print(j);  Serial.print("  ");
                    Serial.println(factors[j]);
                  #endif
                  j = j+1; 
                }
        }
        // choose randomly among possible factors stored in factors:
              k = random(0,j);
              *Term2 = factors[k]; 
              #ifdef FEATURE_SERIAL_MATH
                Serial.print("k, Term2 ");Serial.print(k);Serial.print("  ");Serial.println(*Term2);
              #endif 
            *Term1 = Term0 * *Term2; 
      }   
     else // Term0 = 0
        {
           *Term1 = 0;
           *Term2 = random(1,10);
        }
}

////////////////////////////////////////////////////////////////////////////
// https://forum.arduino.cc/t/printing-a-double-variable/44327

void printDouble( double val, unsigned int precision){

// prints val with number of decimal places determined by precision
// NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
// example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)

    Serial.print (int(val));  //prints the int part
    Serial.print("."); // print the decimal point
    unsigned int frac;
    if(val >= 0)
        frac = (val - int(val)) * precision;
    else
        frac = (int(val)- val ) * precision;
    Serial.println(frac,DEC) ;
} 


void LcdMorse(int num)
{
  switch (num) {
    case 1:
      lcd.print((char)165);lcd.print("-");lcd.print("-");lcd.print("-");lcd.print("-");
      break;
    case 2:
      lcd.print((char)165);lcd.print((char)165);lcd.print("-");lcd.print("-");lcd.print("-");
      break;
    case 3:
      lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);lcd.print("-");lcd.print("-");
      break; 
    case 4:
      lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);lcd.print("-");
      break;  
    case 5:
      lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);
      break;
    case 6:
      lcd.print("-");lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);
      break; 
    case 7:
      lcd.print("-");lcd.print("-");lcd.print((char)165);lcd.print((char)165);lcd.print((char)165);
      break; 
    case 8:
      lcd.print("-");lcd.print("-");lcd.print("-");lcd.print((char)165);lcd.print((char)165);
      break; 
    case 9:
       lcd.print("-");lcd.print("-");lcd.print("-");lcd.print("-");lcd.print((char)165);
       break; 
    default:
      lcd.print("-");lcd.print("-");lcd.print("-");lcd.print("-");lcd.print("-");
      break;                
  }
}

////////////////////////////////////////////////////////////////

void LCDPlanetData(float altitudePlanet, float azimuthPlanet, float phase, float magnitude)
{
    PrintFixedWidth(lcd, (int)round(altitudePlanet), 3);lcd.print(" ");PrintFixedWidth(lcd, (int)round(azimuthPlanet), 3);
    lcd.print(" "); PrintFixedWidth(lcd, (int)round(100*phase), 3); lcd.print(" "); 
    if (magnitude >=0) {lcd.print("+");} // instead of minus sign
    
    if (abs(magnitude) < 10)  lcd.print(String(magnitude, 1));
    else                      lcd.print(String(magnitude, 0)); 
                  
}

////////////////////////////////////////////////////////////////

void LCDChemicalElement(int Hr, int Mn, int Sec)
//
// print 1 or 2-letter chemical abbreviation
{
const char Elements[60][3] = {{"  "}, {"H "}, {"He"}, {"Li"}, {"Be"}, {"B "}, {"C "}, {"N "}, {"O "}, {"F "}, {"Ne"},
                              {"Na"}, {"Mg"}, {"Al"}, {"Si"}, {"P "}, {"S "}, {"Cl"}, {"Ar"}, {"K "}, {"Mg"},
                              {"Sc"}, {"Ti"}, {"V "}, {"Cr"}, {"Mn"}, {"Fe"}, {"Co"}, {"Ni"}, {"Cu"}, {"Zn"}, 
                              {"Ga"}, {"Ge"}, {"As"}, {"Se"}, {"Br"}, {"Kr"}, {"Rb"}, {"Sr"}, {"Y "}, {"Zr"},
                              {"Nb"}, {"Mo"}, {"Tc"}, {"Ru"}, {"Rh"}, {"Pd"}, {"Ag"}, {"Cd"}, {"In"}, {"Sn"},
                              {"Sb"}, {"Te"}, {"I "}, {"Xe"}, {"Cs"}, {"Ba"}, {"La"}, {"Ce"}, {"Pr"}}; 

// Look up 2-letter chemical element from periodic table of elements for local time
      lcd.print(Elements[Hr]);  lcd.print(":"); 
      lcd.print(Elements[Mn]);  lcd.print(":"); 
      lcd.print(Elements[Sec]); lcd.print(" ");
}

////////////////////////////////////////////////////////////////

const char ElementNames[][13] PROGMEM = {
  "Hydrogen    ", "Helium      ", "Lithium     ", "Beryllium   ", "Boron       ", 
  "Carbon      ", "Nitrogen    ", "Oxygen      ", "Fluorine    ", "Neon        ",
  "Sodium      ", "Magnesium   ", "Aluminum    ", "Silicon     ", "Phosphorus  ",
  "Sulfur      ", "Chlorine    ", "Argon       ", "Potassium   ", "Calcium     ",
  "Scandium    ", "Titanium    ", "Vanadium    ", "Chromium    ", "Manganese   ",
  "Iron        ", "Cobalt      ", "Nickel      ", "Copper      ", "Zinc        ",
  "Gallium     ", "Germanium   ", "Arsenic     ", "Selenium    ", "Bromine     ",
  "Krypton     ", "Rubidium    ", "Strontium   ", "Yttrium     ", "Zirconium   ",
  "Niobium     ", "Molybdenum  ", "Technetium  ", "Ruthenium   ", "Rhodium     ",
  "Palladium   ", "Silver      ", "Cadmium     ", "Indium      ", "Tin         ",
  "Antimony    ", "Tellurium   ", "Iodine      ", "Xenon       ", "Cesium      ",
  "Barium      ", "Lanthanum   ", "Cerium      ", "Praseodymium"};

const char  ElementNavn[][13] PROGMEM = {
  "Hydrogen    ", "Helium      ", "Litium      ", "Beryllium   ", "Bor         ", 
  "Karbon      ", "Nitrogen    ", "Oksygen     ", "Fluor       ", "Neon        ",
  "Natrium     ", "Magnesium   ", "Aluminium   ", "Silisium    ", "Fosfor      ",
  "Svovel      ", "Klor        ", "Argon       ", "Kalium      ", "Kalsium     ",
  "Scandium    ", "Titan       ", "Vanadium    ", "Krom        ", "Mangan      ",
  "Jern        ", "Kobolt      ", "Nikkel      ", "Kobber      ", "Sink        ",
  "Gallium     ", "Germanium   ", "Arsen       ", "Selen       ", "Brom        ",
  "Krypton     ", "Rubidium    ", "Strontium   ", "Yttrium     ", "Zirkonium   ",
  "Niob        ", "Molybden    ", "Technetium  ", "Ruthenium   ", "Rhodium     ",
  "Palladium   ", "Solv        ", "Kadmium     ", "Indium      ", "Tinn        ",
  "Antimon     ", "Tellur      ", "Jod         ", "Xenon       ", "Cesium      ",
  "Barium      ", "Lantan      ", "Cerium      ", "Praseodym   "};  

void LCDChemicalElementName(int ElementNo) {

  if (ElementNo >= 1 && ElementNo <= 59) {
    if ((languageNumber >=0) && !(strcmp(languages[languageNumber],"no"))) 
      if (ElementNo == 47) 
      {
        strcpy(textBuffer,"Solv      "); textBuffer[1] = char(NO_DK_oe_SMALL); // sølv
      }        
      else strncpy_P(textBuffer, ElementNavn[ElementNo - 1], 12);    // Norwegian
      
    else   strncpy_P(textBuffer, ElementNames[ElementNo - 1], 12);   // English
          
          textBuffer[12] = '\0';  // end here as there is some rubbish in array beyond desired string ...
          lcd.print(textBuffer);
          
  } else  lcd.print(F("            ")); // empty for element 0

}


////////////////////////////////////////////////////////////////

void LCDChemicalGroupPeriod(int ElementNo)
//
// find group and period in periodic system from chemical Element number
{
int group = 0, period = 0;
      if (ElementNo == 1)
      {
        group = ElementNo;   period = 1;
      }
      else if (ElementNo <= 2)
      {
        group = 18; period = 1;
      }
      else if (ElementNo <= 4)
      {
        group = ElementNo-2; period = 2;
      }
      else if (ElementNo <= 10)
      {
        group = ElementNo+8; period = 2;
      }
      else if (ElementNo <= 12)
      {
        group = ElementNo-10; period = 3;
      }
      else if (ElementNo <= 18)
      {
        group = ElementNo; period = 3;
      }
      else if (ElementNo <= 36)
      {
        group = ElementNo-18; period = 4;
      }
      else if (ElementNo <= 54)
      {
        group = ElementNo-36; period = 5;
      }
      else if (ElementNo <= 56)
      {
        group = ElementNo-54; period = 6;
      }

      if (ElementNo == 0)
      {
        lcd.print(F("           ")); // empty
      }
      else if (ElementNo <= 56)
      {
        lcd.print(F("Gr ")); PrintFixedWidth(lcd, group, 2); lcd.print(" ");
        lcd.print(F("Per ")); lcd.print(period); lcd.print(" ");
      }
      else
      {
        lcd.print(F("Gr  -")); lcd.print(" ");  // no group for Lanthanides
        lcd.print(F("Per 6")); lcd.print(" ");
      }
      
}

////////////////////////////////////

//https://www.instructables.com/two-ways-to-reset-arduino-in-software/
void(*resetFunc)(void) = 0; // declare reset function @ address 0


//////////////////////////////////////////////////////////////////////////////////

int readIntFromEEPROM(int address)
// from https://roboticsbackend.com/arduino-store-int-into-eeprom/
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void updateIntIntoEEPROM(int address, int number)
// modified from writeIntIntoEEPROM from https://roboticsbackend.com/arduino-store-int-into-eeprom/
{ 
  EEPROM.update(address, number >> 8);
  EEPROM.update(address + 1, number & 0xFF);
}


////////////////////////////////////////////////////////////
void InitScreenSelect()
{

noOfStates = 0;

while ((menuStruct[subsetMenu].order[noOfStates] >= 0) && (noOfStates <= lengthOfMenuIn))
    noOfStates = noOfStates + 1;  

  // initialize and unroll menu system order
  for (iiii = 0; iiii < sizeof(menuOrder)/sizeof(menuOrder[0]); iiii += 1) menuOrder[iiii] = -1; // fix 5.10.2022
  for (iiii = 0; iiii < noOfStates; iiii += 1) menuOrder[menuStruct[subsetMenu].order[iiii]] = iiii;
}

//////////////////////////////////////////

int secondaryMenuNumber;

void RotarySecondarySetup(){ // June 2023
// menu system for secondary menu
  uint32_t startTime; // for time-out out of menu
  int toggleInternRotary = 0;

  lcd.setCursor(0,2);
  lcd.print(secondaryMenuNumber);lcd.print(" ");
  
  switch (secondaryMenuNumber) {

//case 0: lcd.print(F("< 0 GPS baud rate > ")); break;
//case 1: lcd.print(F("< 1 FancyClock help>")); break;

case 0: // 00000000 GPS baud rate //////////////
 {
  baudRateNumber = readIntFromEEPROM(9); 
  int oldBaudRateNumber = baudRateNumber;
  
  int noOfMenuIn = sizeof(gpsBaud1)/sizeof(gpsBaud1[1]); 
  
  lcd.setCursor(0,2); PrintFixedWidth(lcd,baudRateNumber, 2); lcd.print(" "); PrintFixedWidth(lcd, gpsBaud1[baudRateNumber], 6);
  startTime = millis();
  while (toggleInternRotary == 0)
   {
  // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();  
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              
        baudRateNumber = baudRateNumber - 1;
        if (baudRateNumber < 0) baudRateNumber = baudRateNumber + noOfMenuIn;
      }
      else if (rotaryResult == r.clockwise()){               
        baudRateNumber = baudRateNumber + 1;
        if (baudRateNumber >= noOfMenuIn) baudRateNumber = baudRateNumber - noOfMenuIn;
      }
      lcd.setCursor(0,2); PrintFixedWidth(lcd,baudRateNumber, 2); lcd.print(" "); PrintFixedWidth(lcd, gpsBaud1[baudRateNumber], 6);
      startTime = millis();  // reset counter if rotary is moved
    } 

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }
    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while

     updateIntIntoEEPROM(9, baudRateNumber);
     if (baudRateNumber != oldBaudRateNumber)  
     {
        delay(500);
        resetFunc();  // call reset if value has changed 
     }   
 
  lcd.clear();
  break;
} // case 0 // baudrate


 case 1: // 1111111111 time on per minute for normal clock in most fancy clock displays ////////////////// 
 {
  secondsClockHelp = EEPROM.read(11);  
  lcd.setCursor(0,2); PrintFixedWidth(lcd, secondsClockHelp, 3); lcd.print(F(" sec per min"));
  startTime = millis();
  while (toggleInternRotary == 0)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();   
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              // decrease  value
        secondsClockHelp = max(secondsClockHelp - 6,   0);
      }
      else if (rotaryResult == r.clockwise()){               // increase  value
        secondsClockHelp = min(secondsClockHelp + 6, 60);
      }
    lcd.setCursor(0,2); PrintFixedWidth(lcd, secondsClockHelp, 3); lcd.print(F(" sec per min"));
    startTime = millis();  // reset counter if rotary is moved
    }

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }

    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while
  
  EEPROM.update(11, secondsClockHelp);
  lcd.clear();
  break;
 } // case 1: secondsClockHelp 

 case 2: // 222222222 no of seconds per screen as DemoClock cycles through all screen //////////////
 {
  dwellTimeDemo = EEPROM.read(12);  
  lcd.setCursor(0,2); PrintFixedWidth(lcd, dwellTimeDemo, 3); lcd.print(F(" sec per screen"));
  startTime = millis();
  while (toggleInternRotary == 0)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();   
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              // decrease  value
        dwellTimeDemo = max(dwellTimeDemo - 1,  5);
      }
      else if (rotaryResult == r.clockwise()){               // increase  value
        dwellTimeDemo = min(dwellTimeDemo + 1, 60);
      }
    lcd.setCursor(0,2); PrintFixedWidth(lcd, dwellTimeDemo, 3); lcd.print(F(" sec per screen"));
    startTime = millis();  // reset counter if rotary is moved
    }

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }

    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while
  
  EEPROM.update(12, dwellTimeDemo);
  lcd.clear();
  break;
 } // case 2: dwellTimeDemo 




 case 3: // 3333333 no of seconds per math quiz //////////////
 {
  mathSecondPeriod = EEPROM.read(13);  
  lcd.setCursor(0,2); PrintFixedWidth(lcd, mathSecondPeriod, 3); lcd.print(F(" sec per quiz  "));
  startTime = millis();
  while (toggleInternRotary == 0)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();   
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              // decrease  value
        mathSecondPeriod = max(mathSecondPeriod - 1,  1);
      }
      else if (rotaryResult == r.clockwise()){               // increase  value
        mathSecondPeriod = min(mathSecondPeriod + 1, 60);
      }
    lcd.setCursor(0,2); PrintFixedWidth(lcd, mathSecondPeriod, 3); lcd.print(F(" sec per quiz  "));
    startTime = millis();  // reset counter if rotary is moved
    }

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }

    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while
  
  EEPROM.update(13, mathSecondPeriod);
  lcd.clear();
  break;
 } // case 3: mathSecondPeriod

 default:  
      lcd.clear();
      return;            // exit! 
 }
}
/////////////////// ROTARY -> SETUP PARAMETERS ///////////////////

void RotarySetup()  //  May-June 2023
{
  int menuNumber = 0; 
  int maxMenuNumber = 5;  // for the 0-5 cases below
  int toggleInternRotary = 0;

  uint32_t startTime; // for time-out out of menu

  #ifdef FEATURE_SERIAL_TIME   // OK her   
        Serial.print(F("A: timeZoneNumber "));Serial.print(timeZoneNumber);Serial.print(F(" "));Serial.println(tcr[timeZoneNumber] -> abbrev);
  #endif
  
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(F("< 0 Clock subset >  ")); // menuNumber = 0

// Top-level: Select which menu to enter: 

    startTime = millis();
    while (toggleInternRotary == 0)
    {
      volatile unsigned char rotaryResultTop = r.process();
      if (rotaryResultTop) {
        if (rotaryResultTop == r.counterClockwise()) { 
          menuNumber = menuNumber - 1;
          if (menuNumber < 0) menuNumber = menuNumber + maxMenuNumber + 1;
        }
        else if (rotaryResultTop == r.clockwise()){   
          menuNumber = menuNumber + 1;
          if (menuNumber > maxMenuNumber) menuNumber = menuNumber - maxMenuNumber - 1;
        }
        lcd.setCursor(0,0);        
        switch(menuNumber) {
        case 0: lcd.print(F("< 0 Clock subset >  ")); break;
        case 1: lcd.print(F("< 1 Backlight >     ")); break;
        case 2: lcd.print(F("< 2 Date format >   ")); break;
        case 3: lcd.print(F("< 3 Time zone >     ")); break;
        case 4: lcd.print(F("< 4 Local language >")); break;
        case 5: lcd.print(F("< 5 Secondary menu >")); break;
         }
        startTime = millis();  // reset counter if rotary is moved
        }
        if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          menuNumber = 99; // do nothing
          return;  // time-out
        }
    
     if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
            toggleInternRotary = 1;                       // only two states
     }
    } // while
///// end top-level /////////////

  #ifdef FEATURE_SERIAL_TIME   // problem allerede her   
        Serial.print(F("B: timeZoneNumber "));Serial.print(timeZoneNumber);Serial.print(F(" "));Serial.println(tcr[timeZoneNumber] -> abbrev);
  #endif


switch (menuNumber) {

case 0: // 0000000000 subset of clock menu ////////////////////////
{
  subsetMenu = readIntFromEEPROM(1); // read int from addresses 1 and 2
  startTime = millis();
  int noOfMenuIn = sizeof(menuStruct)/sizeof(menuStruct[0]);
  
  noOfStates = 0;
  while ((menuStruct[subsetMenu].order[noOfStates] >= 0) && (noOfStates <= lengthOfMenuIn))
        noOfStates = noOfStates + 1;                // find no of entries in this submenu
  lcd.setCursor(0,1); lcd.print(subsetMenu);lcd.print(" ");lcd.print(menuStruct[subsetMenu].descr);
  lcd.print(" (");PrintFixedWidth(lcd, noOfStates, 2); lcd.print(")");
  
  while (toggleInternRotary == 1)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResultSubset = r.process();   
    if (rotaryResultSubset) {
    if (rotaryResultSubset == r.counterClockwise()) {              
        subsetMenu = subsetMenu - 1;
          if (subsetMenu < 0) subsetMenu = subsetMenu + noOfMenuIn ;
      }
      else if (rotaryResultSubset == r.clockwise()){               
        subsetMenu = (subsetMenu + 1);
          if (subsetMenu >= noOfMenuIn) subsetMenu = subsetMenu - noOfMenuIn ;
      }

      noOfStates = 0;
      while ((menuStruct[subsetMenu].order[noOfStates] >= 0) && (noOfStates <= lengthOfMenuIn))
        noOfStates = noOfStates + 1;                // find no of entries in this submenu
      lcd.setCursor(0,1); lcd.print(subsetMenu);lcd.print(" ");lcd.print(menuStruct[subsetMenu].descr);
      lcd.print(" (");PrintFixedWidth(lcd, noOfStates, 2);lcd.print(")");     
      startTime = millis();  // reset counter if rotary is moved
    } 

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          InitScreenSelect();   //  find no of entries in menuIn
          dispState = 0; // go back to first submenu
          return;  // time-out
        }
    if (r.buttonPressedReleased(25)) {               // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while

updateIntIntoEEPROM(1, subsetMenu);
InitScreenSelect();   //  find no of entries in menuIn
dispState = 0; // go back to first submenu
lcd.clear();  
break;
} // case 0: subset of clock menu 


 case 1: // 1111111111 backlight ////////////////// 
 {
  backlightVal = EEPROM.read(0);  
  lcd.setCursor(0,1); PrintFixedWidth(lcd, backlightVal, 6);
  startTime = millis();
  while (toggleInternRotary == 1)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResultBacklight = r.process();   
    if (rotaryResultBacklight) {
    if (rotaryResultBacklight == r.counterClockwise()) {              // decrease backlight value
        backlightVal = max(backlightVal - 10,   0);
      }
      else if (rotaryResultBacklight == r.clockwise()){               // increase backlight value
        backlightVal = min(backlightVal + 10, 250);
      }
    lcd.setCursor(0,1); PrintFixedWidth(lcd, backlightVal, 6);
    analogWrite(LCD_PWM, backlightVal); 
    startTime = millis();  // reset counter if rotary is moved
    }

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }

    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while
  
  EEPROM.update(0, backlightVal);
  lcd.clear();
  break;
 } // case 1: backlight 

 
case 2: // 2222222222 date format ////////////////
// code from here instead ? https://stackoverflow.com/questions/53679924/how-to-constrain-a-value-within-a-range
 { 
  dateFormat = readIntFromEEPROM(3); // read int from EEPROM addresses 3 and 4
  Day = day(local[timeZoneNumber]);
  Month = month(local[timeZoneNumber]);
  Year = year(local[timeZoneNumber]);
  
  lcd.setCursor(0,1); lcd.print(dateFormat); lcd.print(" ");lcd.print(dateTimeFormat[dateFormat].descr);
  lcd.setCursor(0,3); LcdDate(Day, Month, Year);
  sprintf(textBuffer, " %02d%c%02d%c%02d", Hour, dateTimeFormat[dateFormat].hourSep, Minute, dateTimeFormat[dateFormat].minSep, Seconds);
  lcd.print(textBuffer);
  
  int noOfMenuIn = sizeof(dateTimeFormat)/sizeof(dateTimeFormat[0]);

  startTime = millis();
  while (toggleInternRotary == 1)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();   
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              
        dateFormat = dateFormat - 1;
        if (dateFormat < 0) dateFormat = dateFormat + noOfMenuIn;
      }
      else if (rotaryResult == r.clockwise()){               
        dateFormat = dateFormat + 1;
        if (dateFormat >= noOfMenuIn) dateFormat = dateFormat - noOfMenuIn;
      }

    lcd.setCursor(0,1); lcd.print(dateFormat); lcd.print(" ");lcd.print(dateTimeFormat[dateFormat].descr);
    lcd.setCursor(0,3); LcdDate(Day, Month, Year);
    sprintf(textBuffer, " %02d%c%02d%c%02d", Hour, dateTimeFormat[dateFormat].hourSep, Minute, dateTimeFormat[dateFormat].minSep, Seconds);
    lcd.print(textBuffer);

      //lcd.print(dateTimeFormat[dateFormat].dateOrder);  PrintFixedWidth(lcd, dateFormat, 3);
      startTime = millis();  // reset counter if rotary is moved
    } 

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }
    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while

  updateIntIntoEEPROM(3, dateFormat);
  lcd.clear();
  break;
 }  // case 2: date format
 

case 3: // 333333333 time zone /////////////////////////////////////////
{ 
  #ifdef FEATURE_SERIAL_TIME      
//        Serial.print(F("C: timeZoneNumber "));Serial.print(timeZoneNumber);Serial.print(F(" "));Serial.println(tcr[timeZoneNumber] -> abbrev);
  #endif
  
  lcd.setCursor(0,1);
//  numTimeZones is set in clock_zone.h
  timeZoneNumber = readIntFromEEPROM(7); // read int from EEPROM addresses 7 and 8
  if ((timeZoneNumber < 0) || (timeZoneNumber >= numTimeZones)) // if EEPROM stores invalid value
       timeZoneNumber = 0;                                      // set to default value 

  lcd.setCursor(0,1); PrintFixedWidth(lcd, timeZoneNumber, 2); lcd.print(" "); lcd.print(tcr[timeZoneNumber] -> abbrev);lcd.print("  ");

  lcd.setCursor(9,3); lcd.print(F("UTC"));
  utcOffset = local[timeZoneNumber] / long(60) - utc / long(60); // order of calculation is important 
  if (utcOffset >=0)  lcd.print("+");
  lcd.print(float(utcOffset)/60); lcd.print("  ");

  int firstZone = 0;// 1;// 0; first index used in tcr and local: only !=0 for debugging
  
  startTime = millis();
  while (toggleInternRotary == 1)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();   
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              
        timeZoneNumber = timeZoneNumber - 1;
        if (timeZoneNumber < firstZone) timeZoneNumber = timeZoneNumber + numTimeZones - firstZone + 1;
      }
      else if (rotaryResult == r.clockwise()){               
        timeZoneNumber = timeZoneNumber + 1;
        if (timeZoneNumber > numTimeZones) timeZoneNumber = timeZoneNumber - numTimeZones + firstZone - 1;
      }
#ifdef FEATURE_SERIAL_TIME      
//        Serial.print(F("firstZone ")); Serial.print(firstZone); Serial.print(F(" numTimeZones "));Serial.print(numTimeZones);
        Serial.print(F("Z: timeZoneNumber "));Serial.print(timeZoneNumber);Serial.print(F(" "));Serial.println(tcr[timeZoneNumber] -> abbrev);
#endif
      
      lcd.setCursor(0,1); PrintFixedWidth(lcd, timeZoneNumber, 2); lcd.print(" "); 
      lcd.print(tcr[timeZoneNumber] -> abbrev);lcd.print("  ");
      utcOffset = local[timeZoneNumber] / long(60) - utc / long(60); // order of calculation is important
      lcd.setCursor(9,3); lcd.print(F("UTC")); 
      if (utcOffset >=0)  lcd.print("+");
      lcd.print(float(utcOffset)/60); lcd.print("  ");
  
      startTime = millis();  // reset counter if rotary is moved
    } 

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }
    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable to quit while-loop
    }   
  } // while
 
  updateIntIntoEEPROM(7, timeZoneNumber);
  lcd.clear();
  break;
}  // case 3: time zone 


 case 4: // 4444444 local language for day names /////////////////////
 {
  
  lcd.setCursor(0,1);
  byte numLanguages = sizeof(languages) / sizeof(languages[0]);
  languageNumber = readIntFromEEPROM(5); // read int from EEPROM addresses 5 and 6
  
  lcd.setCursor(4,1);
  if (languageNumber >=0) lcd.print(languages[languageNumber]);
  else                    lcd.print(F("en"));

  if (languageNumber >=0)
    {
    nativeDayLong(local[timeZoneNumber]);
    sprintf(todayFormatted,"%12s", today);
    }
  else // English
    sprintf(todayFormatted, "%12s", dayStr(weekday(local[timeZoneNumber])));  // normal
    lcd.setCursor(0,3);; lcd.print(todayFormatted);
 
  startTime = millis();
  while (toggleInternRotary == 1)
  { 
   // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResultDate = r.process();   
    if (rotaryResultDate) {
    if (rotaryResultDate == r.counterClockwise()) {              
        languageNumber = languageNumber - 1;
        if (languageNumber < -1) languageNumber = languageNumber + numLanguages+1;
      }
      else if (rotaryResultDate == r.clockwise()){               
        languageNumber = languageNumber + 1;
          if (languageNumber >= numLanguages) languageNumber = languageNumber - numLanguages - 1;
      }
   
      // debug  
      /*
      Serial.print(languageNumber); Serial.print(" "); 
      nativeDayLong(1111115);
      if (languageNumber >=0) 
      {
        Serial.print(languages[languageNumber]); Serial.print(" "); 
        Serial.println(today);
      }
      else
      {
        Serial.println("en");
      }
      */
      // debug 
      startTime = millis();  // reset counter if rotary is moved
    }
    
    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }    
  
    lcd.setCursor(4,1); 
    if (languageNumber >=0) lcd.print(languages[languageNumber]);
    else                    lcd.print(F("en"));

    if (languageNumber >=0)
    {
      nativeDayLong(local[timeZoneNumber]);
      sprintf(todayFormatted,"%12s", today);
    }
    else // English
      sprintf(todayFormatted, "%12s", dayStr(weekday(local[timeZoneNumber])));  // normal
    lcd.setCursor(0,3);; lcd.print(todayFormatted);
 
    
    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        toggleInternRotary = toggleInternRotary + 1; // internal variable
        lcd.clear();
    }   
  } // while

  updateIntIntoEEPROM(5, languageNumber); 
  lcd.clear();
  break;
  
 }  // case 4: local language 
 
case 5: // 55555555 Secondary menu //////////////
 {
  int noOfMenuIn = 4; 
  lcd.setCursor(0,1); lcd.print(F("<<GPS baudrate   >>   "));
  
  startTime = millis();
  secondaryMenuNumber = 0;
  while (toggleInternRotary == 1)
   {
  // During each loop, check the encoder to see if it has been changed.
    volatile unsigned char rotaryResult = r.process();  
    if (rotaryResult) {
    if (rotaryResult == r.counterClockwise()) {              
        secondaryMenuNumber = secondaryMenuNumber - 1;
        if (secondaryMenuNumber < 0) secondaryMenuNumber = secondaryMenuNumber + noOfMenuIn;
      }
      else if (rotaryResult == r.clockwise()){               
        secondaryMenuNumber = secondaryMenuNumber + 1;
        if (secondaryMenuNumber >= noOfMenuIn) secondaryMenuNumber = secondaryMenuNumber - noOfMenuIn;
      }
      //lcd.setCursor(0,1); lcd.print(secondaryMenuNumber); 
      lcd.setCursor(0,1);
      switch (secondaryMenuNumber) { 
      case 0: lcd.print(F("<< GPS baudrate >>  ")); break;
      case 1: lcd.print(F("<< FancyClock help>>")); break;
      case 2: lcd.print(F("<< Dwell time demo>>")); break;
      case 3: lcd.print(F("<< Time, math quiz>>")); break;
      }
      startTime = millis();  // reset counter if rotary is moved
    } 

    if (millis() - startTime > menuTimeOut) // check for time-out and return
        {
          lcd.clear();
          return;  // time-out
        }
    if (r.buttonPressedReleased(25)) {            // 25ms = debounce_delay
        RotarySecondarySetup();  // secondaryMenuNumber is interpreted by RotarySecondarySetup
        return;
    }   
  } // while

  //lcd.clear();
  break;
} // case 5 // secondary menu


default:  
      return;            // exit!                   
      
} // switch
  
}




 /// THE END ///
