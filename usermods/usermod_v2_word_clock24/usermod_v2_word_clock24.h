#pragma once

#include "wled.h"

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This usermod can be used to drive a wordclock with a 11x10 pixel matrix with WLED. There are also 4 additional dots for the minutes. 
 * The visualisation is desribed in 4 mask with LED numbers (single dots for minutes, minutes, hours and "clock/Uhr").
 * There are 2 parameters to chnage the behaviour:
 * 
 * active: enable/disable usermod
 * diplayItIs: enable/disable display of "Es ist" on the clock.
 */

class WordClock24Usermod: public Usermod 
{ // 12.03.2023 ToDo: 
  // Die Angabe UHR immmer nur zur vollen Stunde einblenden!
  // Die Angabe UHR erscheint nicht bei Minute 0 sondern erst bei Minute 1.
  // "P" LED 0 (unten links) leuchtet im Meander-Modus 

  enum eDIALECT
  {
    FIRST,
    NONE = FIRST,
    SAXONY,
    SWISS,
    LAST
  };

  private:
    unsigned long lastTime = 0;
    int lastTimeMinutes =   -1;

    // set your config variables to their boot default value (this can also be done in readFromConfig() or a constructor if you prefer)
    bool usermodActive = true;
    bool displayItIs = true; // ES IST ... UHR 
    eDIALECT m_eDialect = eDIALECT::NONE;

    
    // defines for mask sizes
    #define maskSizeLeds        121 
    #define maskSizeMinutes     12
    #define maskSizeHours       14
    #define maskSizeHoursDia    15
    #define maskSizeItIs        9
    #define maskSizeMinuteDots  4

    // "minute" masks
    // Normal wiring
    const int maskMinutes[12][maskSizeMinutes] = 
    { {   8,   9,   10,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 }, // :00
      {   66,  67,  68,  69, 106, 107, 108, 109,  -1,  -1,  -1,  -1 }, // :05 fünf nach
      {   66,  67,  68,  69, 117, 118, 119, 120,  -1,  -1,  -1,  -1,}, // :10 zehn nach
      {   92,  93,  94,  95,  96,  97,  98,  -1,  -1,  -1,  -1,  -1 }, // :15 viertel
      {   62,  63,  64,  65,  74,  75,  76, 117, 118, 119, 120,  -1 }, // :20 zehn vor halb
      {   62,  63,  64,  65,  74,  75,  76, 106, 107, 108, 109,  -1 }, // :25 fünf vor halb
      {   62,  63,  64,  65,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 }, // :30 halb
      {   62,  63,  64,  65,  66,  67,  68,  69, 106, 107, 108, 109 }, // :35 fünf nach halb
      {   62,  63,  64,  65,  66,  67,  68,  69, 117, 118, 119, 120 }, // :40 zehn nach halb
      {   88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  -1 }, // :45 dreiviertel
      {   74,  75,  76, 117, 118, 119, 120,  -1,  -1,  -1,  -1,  -1 }, // :50 zehn vor
      {   74,  75,  76, 106, 107, 108, 109,  -1,  -1,  -1,  -1,  -1 }, // :55 fünf vor 
    };

    // Meander wiring for saxyon
    const int maskMinutesDialect[12][maskSizeMinutes] =
    { {  8,  9,  10,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 }, // :00
      {  66,  67,  68,  69, 106, 107, 108, 109,  -1,  -1,  -1,  -1 }, // :05 fünf nach
      {  66,  67,  68,  69, 117, 118, 119, 120,  -1,  -1,  -1,  -1 }, // :10 zehn nach
      {  66,  67,  68,  69,  92,  93,  94,  95,  96,  97,  98,  -1 }, // :15 viertel nach   
      {  66,  67,  68,  69,  77,  78,  79,  80,  81,  82,  83,  -1 }, // :20 zwanzig nach
      {  62,  63,  64,  65,  74,  75,  76, 106, 107, 108, 109,  -1 }, // :25 fünf vor halb
      {  62,  63,  64,  65,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 }, // :30 halb
      {  62,  63,  64,  65,  66,  67,  68,  69, 106, 107, 108, 109 }, // :35 fünf nach halb
      {  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  -1,  -1 }, // :40 zwanzig vor
      {  74,  75,  76,  92,  93,  94,  95,  96,  97,  98,  -1,  -1 }, // :45 viertel vor
      {  74,  75,  76, 117, 118, 119, 120,  -1,  -1,  -1,  -1,  -1 }, // :50 zehn vor
      {  74,  75,  76, 106, 107, 108, 109,  -1,  -1,  -1,  -1,  -1 }, // :55 fünf vor 
    };

    // hour masks
    // Normal wiring
    const int maskHours[25][maskSizeHours] = 
    { {  51,  52,  53,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 01: ein
      {  51,  52,  53,  54,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 01: eins
      {  44,  45,  46,  47,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 02: zwei
      {  49,  50,  51,  52,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 03: drei
      {  29,  30,  31,  32,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 04: vier
      {  33,  34,  35,  36,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 05: fünf
      {  39,  40,  41,  42,  43,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 06: sechs
      {  55,  56,  57,  58,  59,  60,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 07: sieben
      {  17,  18,  19,  20,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 08: acht
      {  25,  26,  27,  28,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 09: neun
      {  13,  14,  15,  16,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 10: zehn
      {  22,  23,  24,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 11: elf
      {   1,   2,   3,   4,   5,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 12: zwölf 
      {  13,  14,  15,  16,  49,  50,  51,  52, -1, -1,  -1,  -1,  -1,  -1 }, // 13: dreizehn 
      {  13,  14,  15,  16,  29,  30,  31,  32, -1, -1,  -1,  -1,  -1,  -1 }, // 14: vierzehn
      {  13,  14,  15,  16,  33,  34,  35,  36, -1, -1,  -1,  -1,  -1,  -1 }, // 15: fünfzehn
      {  13,  14,  15,  16,  40,  41,  42,  43, -1, -1,  -1,  -1,  -1,  -1 }, // 16: sechzehn 
      {  13,  14,  15,  16,  57,  58,  59,  60, -1, -1,  -1,  -1,  -1,  -1 }, // 17: siebzehn
      {  13,  14,  15,  16,  17,  18,  19,  20, -1, -1,  -1,  -1,  -1,  -1 }, // 18: achtzehn 
      {  13,  14,  15,  16,  25,  26,  27,  28, -1, -1,  -1,  -1,  -1,  -1 }, // 19: neunzehn
      {  77,  78,  79,  80,  81,  82,  83,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 20: zwanzig 
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86, 101, 102, 103,  -1 }, // 21: einundzwanzig
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86, 102, 103, 104, 105 }, // 22: zweiundzwanzig 
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86,  88,  89,  90,  91 }, // 23: dreiundzwanzig
      {   1,   2,   3,   4,   5,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 24: zwölf Uhr Mitternacht
    };

    // Meander wiring
    const int maskHoursSaxony[24][maskSizeHoursDia] = 
    { {  51,  52,  53,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 01: ein
      {  51,  52,  53,  54,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 01: eins
      {  44,  45,  46,  47,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 02: zwei
      {  49,  50,  51,  52,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 03: drei
      {  29,  30,  31,  32,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 04: vier
      {  33,  34,  35,  36,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 05: fünf
      {  39,  40,  41,  42,  43,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 06: sechs
      {  55,  56,  57,  58,  59,  60,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 07: sieben
      {  17,  18,  19,  20,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 08: acht
      {  25,  26,  27,  28,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 09: neun
      {  13,  14,  15,  16,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 10: zehn
      {  22,  23,  24,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 11: elf
      {   1,   2,   3,   4,   5,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 12: zwölf 
      {  13,  14,  15,  16,  49,  50,  51,  52, -1, -1,  -1,  -1,  -1,  -1 }, // 13: dreizehn 
      {  13,  14,  15,  16,  29,  30,  31,  32, -1, -1,  -1,  -1,  -1,  -1 }, // 14: vierzehn
      {  13,  14,  15,  16,  33,  34,  35,  36, -1, -1,  -1,  -1,  -1,  -1 }, // 15: fünfzehn
      {  13,  14,  15,  16,  43,  44,  45,  46, -1, -1,  -1,  -1,  -1,  -1 }, // 16: sechzehn 
      {  13,  14,  15,  16,  71,  73,  74,  75, -1, -1,  -1,  -1,  -1,  -1 }, // 17: siebzehn
      {  13,  14,  15,  16,  17,  18,  19,  20, -1, -1,  -1,  -1,  -1,  -1 }, // 18: achtzehn 
      {  13,  14,  15,  16,  25,  26,  27,  28, -1, -1,  -1,  -1,  -1,  -1 }, // 19: neunzehn
      {  77,  78,  79,  80,  81,  82,  83,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 20: zwanzig 
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86, 101, 102, 103,  -1 }, // 21: einundzwanzig
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86, 102, 103, 104, 105 }, // 22: zweiundzwanzig 
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86,  88,  89,  90,  91 }  // 23: dreiundzwanzig
    };

    // Meander wiring hours
    const int maskHoursSwiss[24][maskSizeHoursDia] = 
    { {  51,  52,  53,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 01: ein ?
      {  66,  65,  64,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 01: eins !
      {  63,  62,  61,  60,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 02: zwei !
      {  58,  57,  56,  -1,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 03: drei !
      {  55,  54,  53,  52,  51,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 04: vier !
      {  50,  49,  48,  47,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 05: fünf !
      {  44,  43,  42,  41,  40,  39,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 06: sechs !
      {  38,  37,  36,  35,  34,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 07: sieben !
      {  23,  24,  25,  26,  27,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 08: acht !
      {  28,  29,  30,  31,  32,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 09: neun !
      {  22,  21,  20,  19,  18,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 10: zehn !
      {  15,  14,  13,  12,  -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 11: elf !
      {   1,   2,   3,   4,   5,  6,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 12: zwölf !
      {  13,  14,  15,  16,  49,  50,  51,  52, -1, -1,  -1,  -1,  -1,  -1 }, // 13: dreizehn 
      {  13,  14,  15,  16,  29,  30,  31,  32, -1, -1,  -1,  -1,  -1,  -1 }, // 14: vierzehn 
      {  13,  14,  15,  16,  33,  34,  35,  36, -1, -1,  -1,  -1,  -1,  -1 }, // 15: fünfzehn 
      {  13,  14,  15,  16,  43,  44,  45,  46, -1, -1,  -1,  -1,  -1,  -1 }, // 16: sechzehn 
      {  13,  14,  15,  16,  71,  73,  74,  75, -1, -1,  -1,  -1,  -1,  -1 }, // 17: siebzehn
      {  13,  14,  15,  16,  17,  18,  19,  20, -1, -1,  -1,  -1,  -1,  -1 }, // 18: achtzehn 
      {  13,  14,  15,  16,  25,  26,  27,  28, -1, -1,  -1,  -1,  -1,  -1 }, // 19: neunzehn
      {  77,  78,  79,  80,  81,  82,  83,  -1, -1, -1,  -1,  -1,  -1,  -1 }, // 20: zwanzig 
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86, 101, 102, 103,  -1 }, // 21: einundzwanzig
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86, 102, 103, 104, 105 }, // 22: zweiundzwanzig 
      {  77,  78,  79,  80,  81,  82,  83,  84, 85, 86,  88,  89,  90,  91 }  // 23: dreiundzwanzig
    };
 
    int maskItIs[maskSizeItIs]      = { 110, 111, 113, 114, 115,  -1,  -1,  -1, -1};  // E S  I S T 
    int maskItIsSwiss[maskSizeItIs] = { 110, 111, 113, 114, 115, 116,  -1,  -1, -1};  // E S  ISCH
    int maskItIsSaxony[maskSizeItIs] = { 111, 112, 113, 114, 116, 117,  118,  119, 120};  // ITZE ISSES

    // UHR ein- / ausblenden
    void set_oClock (int minute) 
    { 
     if (minute < 4) // Nick , funktioniert, klingt aber für uns nicht logisch (von 5 auf 4 geändert)
      {
        maskItIs[5] =  8;  // U
        maskItIs[6] =  9;  // H
        maskItIs[7] = 10;  // R
      }
     else
      {  
        maskItIs[5] = -1;
        maskItIs[6] = -1;
        maskItIs[7] = -1;
      }
    }

    // mask minute dots
    const int maskMinuteDots[maskSizeMinuteDots] = { 70, 71, 72, 73 };
    const int maskMinuteDotsSaxony[maskSizeMinuteDots] = { 83, 82, 81, 80 };
    // overall mask to define which LEDs are on
    int maskLedsOn[maskSizeLeds] = 
    {
      0,0,0,0,0,0,0,0,0,0,0, 
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0 
    };

    // update led mask
    void updateLedMask(const int wordMask[], int arraySize)
    {
      // loop over array
      for (int x=0; x < arraySize; x++) 
      {
        // check if mask has a valid LED number
        if (wordMask[x] >= 0 && wordMask[x] < maskSizeLeds)
        {
          // turn LED on
          maskLedsOn[wordMask[x]] = 1;
        }
      }
    }

    // set hours
    void setHours(int hours, bool fullClock, int minute)
    {
      int index = hours;

      // handle 00:xx as 12:xx
      if (hours == 0)
      {
        index = 24; // 12 vorher
      }

      // check if we get an overrun of 24 o´clock
      if (hours == 25)
      {
        index = 1;
      }

      // special handling for "ein Uhr" instead of "eins Uhr"
      if (hours == 1 && fullClock == true)
      {
        index = 0;
      }
      // special handling for hours > 12, full text only when full hour
      // wenn Minute > 5 , dann gehe zwölf Stunden zurück
      if (hours > 12){
        if (minute > 4 ) // geändert von > 5 (Nick)
        {
           index = hours - 12;
        };
      }

      // update led mask
      switch(m_eDialect)
      {
        case eDIALECT::SAXONY:
          updateLedMask(maskHoursSaxony[index], maskSizeHoursDia);
          break;
        
        case eDIALECT::SWISS:
          updateLedMask(maskHoursSwiss[index], maskSizeHoursDia);
          break;
        
        default:
          updateLedMask(maskHours[index], maskSizeHours);
          break;
      }
    }

    // set minutes
    void setMinutes(int index)
    {
      // update led mask
       switch(m_eDialect)
      {
        case eDIALECT::SAXONY:
        case eDIALECT::SWISS:
          updateLedMask(maskMinutesDialect[index], maskSizeMinutes);
          break;
        
        default:
          updateLedMask(maskMinutes[index], maskSizeMinutes);
          break;
      }
    }

    // set minutes dot
    void setSingleMinuteDots(int minutes)
    {
      // modulo to get minute dots
      int minutesDotCount = minutes % 5;

      // check if minute dots are active
      if (minutesDotCount > 0)
      {
        // activate all minute dots until number is reached
        for (int i = 0; i < minutesDotCount; i++)
        {
      switch(m_eDialect)
        {
        case eDIALECT::SAXONY:
          maskLedsOn[maskMinuteDotsSaxony[i]] = 1;  
          break;
        default:
          maskLedsOn[maskMinuteDots[i]] = 1;  
          break;
        } 
        }
      }
    }

    // update the display
    void updateDisplay(uint8_t hours, uint8_t minutes) 
    {
      // disable complete matrix at the bigging
      for (int x = 0; x < maskSizeLeds; x++)
      {
        maskLedsOn[x] = 0; // 1: alle an; 0: alle aus
      } 
      
      // display it is/es ist if activated
      if (displayItIs)
      {
        switch(m_eDialect)
        {
          case eDIALECT::SWISS:
            updateLedMask(maskItIsSwiss, maskSizeItIs);
            break;

          case eDIALECT::SAXONY:
          default:
            updateLedMask(maskItIsSaxony, maskSizeItIs);
            break;
        }
      }

      // display UHR, if full hour
      // set_oClock(minute(localTime)); Nick 
      set_oClock(minutes); 
      
      // set single minute dots
      setSingleMinuteDots(minutes);

     // switch minutes
      switch (minutes / 5) 
      {
        case 0:
            // full hour
            setMinutes(0);
            setHours(hours, true, minutes);
            break;
        case 1:
            // 5 nach
            setMinutes(1);
            setHours(hours, false,minutes);
            break;
        case 2:
            // 10 nach
            setMinutes(2);
            setHours(hours, false,minutes);
            break;
        case 3:
            // viertel nach
            setMinutes(3);
            switch(m_eDialect)
            {
              case eDIALECT::SAXONY:
              case eDIALECT::SWISS:
                setHours(hours, false,minutes);
                break;

              default:
                setHours(hours+1, false,minutes);
                break;  
            }
            break;
        case 4:
            // 20 nach
            setMinutes(4);
            switch(m_eDialect)
            {
              case eDIALECT::SAXONY:
              case eDIALECT::SWISS:
                setHours(hours, false,minutes);
                break;

              default:
                setHours(hours+1, false,minutes);
                break;  
            }
            break;
        case 5:
            // 5 vor halb
            setMinutes(5);
            setHours(hours + 1, false,minutes);
            break;
        case 6:
            // halb
            setMinutes(6);
            setHours(hours + 1, false,minutes);
            break;
        case 7:
            // 5 nach halb
            setMinutes(7);
            setHours(hours + 1, false,minutes);
            break;
        case 8:
            // 20 vor oder 10 nach halb
            setMinutes(8);
            setHours(hours + 1, false,minutes);
            break;
        case 9:
            // viertel vor
            setMinutes(9);
            setHours(hours + 1, false,minutes);
            break;
        case 10:
            // 10 vor
            setMinutes(10);
            setHours(hours + 1, false,minutes);
            break;
        case 11:
            // 5 vor
            setMinutes(11);
            setHours(hours + 1, false,minutes);
            break;
        }
    }

  public:
    //Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() 
    {
    }

    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() 
    {
    }

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {

      // do it every 5 seconds
      if (millis() - lastTime > 5000) 
      {
         DEBUG_PRINTLN("Updating WordClock24");
        // check the time
        int minutes = minute(localTime);

        // check if we already updated this minute
        if (lastTimeMinutes != minutes)
        {
          // update the display with new time
          updateDisplay(hour(localTime), minute(localTime));
          //updateDisplay(2, 9);
          // remember last update time
          lastTimeMinutes = minutes;
        }

        // remember last update
        lastTime = millis();
      }
    }

    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    /*
    void addToJsonInfo(JsonObject& root)
    {
    }
    */

    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
    }

    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
    }

    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will make your settings editable through the Usermod Settings page automatically.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
     *
     * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
     * 
     * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.  
     * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject("WordClock24Usermod");
      top["active"] = usermodActive;
      top["ES IST anzeigen"] = displayItIs;
      top["Dialect"] = m_eDialect;
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     * 
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root)
    {
      // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
      // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

      JsonObject top = root["WordClock24Usermod"];

      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top["active"], usermodActive);
      configComplete &= getJsonValue(top["ES IST anzeigen"], displayItIs);
      configComplete &= getJsonValue(top["Dialect"], m_eDialect);

      return configComplete;
    }

    virtual void appendConfigData() override
    {
      oappend(SET_F("addInfo('WordClock24Usermod:Dialect',1,'0=None,1=Saxony,2=Swiss');"));
    }

    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
      // check if usermod is active
      if (usermodActive == true)
      {
        // loop over all leds
        for (int x = 0; x < maskSizeLeds; x++)
        {
          // check mask
          if (maskLedsOn[x] == 0)
          {
            // set pixel off
            strip.setPixelColor(x, RGBW32(0,0,0,0));
          }
        }
      }
    }

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_WORDCLOCK24;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};