#ifndef __DCF77_H__
#define __DCF77_H__

/*
 * This usermod allows you to get date and time from a DCF77 module. This is usefull if 
 * you don't have access to wifi / NTP.
 * The usermod uses a modified version of the DCF77 arduino library https://github.com/thijse/Arduino-DCF77.
 * 
 * The mod is tested with a D1 mini. I have no idea if it works with other controllers.
 * Connect the DCF77 modul as follows:
 *    - V to 3.3V
 *    - GND to GND
 *    - P1 (Enable) to GND
 *    - T (Signal) to e.g. GPIO15 - (D8 on D1 mini) 
 *
 * You HAVE TO set the T (Signal) GPIO in usermod settings over the webinterface.
 * 
 * To enable this mod add it to usermods_list.cpp with the define USERMOD_DCF77.
 * Also add suitable USERMOD_ID_DCF77 to const.h.
 * To enable the module:
 *    add -DUSERMOD_DCF77 to your platformio_override.ini
 *    add -DPIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48 - otherwise you'll get a compile error
 * 
 * If you have any questions, visit us at the star laboratory (sternenlabor) in Plauen.
 */


#include <wled.h>

#define MIN_TIME 1334102400 // Date: 11-4-2012
#define MAX_TIME 4102444800 // Date:  1-1-2100

#define DCFRejectionTime 700   // Pulse-to-Pulse rejection time.
#define DCFRejectPulseWidth 50 // Minimal pulse width
#define DCFSplitTime 180       // Specifications distinguishes pulse width 100 ms and 200 ms. In practice we see 130 ms and 230
#define DCFSyncTime 1500       // Specifications defines 2000 ms pulse for end of sequence

static void IRAM_ATTR int0handler();

time_t now()
{
   return localTime;
}

void printTime(time_t tim)
{
   DEBUG_PRINT(hour(tim));
   DEBUG_PRINT(":");
   DEBUG_PRINT(minute(tim));
   DEBUG_PRINT(":");
   DEBUG_PRINTLN(second(tim));
}

void setTime(time_t tim)
{
   printTime(tim);
   toki.setTime(tim, TOKI_NO_MS_ACCURACY, TOKI_TS_JSON);
}


class DCF77
{
public:
   // DCF time format structure
   struct DCF77Buffer
   {
      // unsigned long long prefix       :21;
      unsigned long long prefix : 17;
      unsigned long long CEST : 1;    // CEST
      unsigned long long CET : 1;     // CET
      unsigned long long unused : 2;  // unused bits
      unsigned long long Min : 7;     // minutes
      unsigned long long P1 : 1;      // parity minutes
      unsigned long long Hour : 6;    // hours
      unsigned long long P2 : 1;      // parity hours
      unsigned long long Day : 6;     // day
      unsigned long long Weekday : 3; // day of week
      unsigned long long Month : 5;   // month
      unsigned long long Year : 8;    // year (5 -> 2005)
      unsigned long long P3 : 1;      // parity
   };

   // DCF Parity format structure
   struct ParityFlags
   {
      unsigned char parityFlag : 1;
      unsigned char parityMin : 1;
      unsigned char parityHour : 1;
      unsigned char parityDate : 1;
   } flags;

   // Parameters shared between interupt loop and main loop
   volatile bool FilledBufferAvailable;
   volatile unsigned long long filledBuffer;
   volatile time_t filledTimestamp;

   // DCF Buffers and indicators
   int bufferPosition;
   unsigned long long runningBuffer;
   unsigned long long processingBuffer;

   // Pulse flanks
   int leadingEdge;
   int trailingEdge;
   int PreviousLeadingEdge;
   bool Up;

   // Private functions
   void initialize(void)
   {
      leadingEdge = 0;
      trailingEdge = 0;
      PreviousLeadingEdge = 0;
      Up = false;
      runningBuffer = 0;
      FilledBufferAvailable = false;
      bufferPosition = 0;
      flags.parityDate = 0;
      flags.parityFlag = 0;
      flags.parityHour = 0;
      flags.parityMin = 0;
      CEST = 0;
   }

   void bufferinit(void)
   {
      runningBuffer = 0;
      bufferPosition = 0;
   }

   void finalizeBuffer(void)
   {
      if (bufferPosition == 59)
      {
         // Buffer is full
         DEBUG_PRINTLN("BF");
         // Prepare filled buffer and time stamp for main loop
         filledBuffer = runningBuffer;
         filledTimestamp = now();
         // Reset running buffer
         bufferinit();
         FilledBufferAvailable = true;
      }
      else
      {
         // Buffer is not yet full at end of time-sequence
         DEBUG_PRINTLN("EoM");
         // Reset running buffer
         bufferinit();
      }
   }

   bool receivedTimeUpdate(void)
   {
      // If buffer is not filled, there is no new time
      if (!FilledBufferAvailable)
      {
         return false;
      }
      // if buffer is filled, we will process it and see if this results in valid parity
      if (!processBuffer())
      {
         DEBUG_PRINTLN("Invalid parity");
         return false;
      }

      // Since the received signal is error-prone, and the parity check is not very strong,
      // we will do some sanity checks on the time
      time_t processedTime = latestupdatedTime + (now() - processingTimestamp);
      if (processedTime < MIN_TIME || processedTime > MAX_TIME)
      {
         DEBUG_PRINTLN("Time outside of bounds");
         return false;
      }

      // If received time is close to internal clock (2 min) we are satisfied
      time_t difference = abs(processedTime - now());
      if (difference < 2 * SECS_PER_MIN)
      {
         DEBUG_PRINTLN("close to internal clock");
         storePreviousTime();
         return true;
      }

      // Time can be further from internal clock for several reasons
      // We will check if lag from internal clock is consistent
      time_t shiftPrevious = (previousUpdatedTime - previousProcessingTimestamp);
      time_t shiftCurrent = (latestupdatedTime - processingTimestamp);
      time_t shiftDifference = abs(shiftCurrent - shiftPrevious);
      storePreviousTime();
      if (shiftDifference < 2 * SECS_PER_MIN)
      {
         DEBUG_PRINTLN("time lag consistent");
         return true;
      }
      else
      {
         DEBUG_PRINTLN("time lag inconsistent");
      }

      // If lag is inconsistent, this may be because of no previous stored date
      // This would be resolved in a second run.
      return false;
   }

   void storePreviousTime(void)
   {
      previousUpdatedTime = latestupdatedTime;
      previousProcessingTimestamp = processingTimestamp;
   }

   void calculateBufferParities(void)
   {
      // Calculate Parity
      flags.parityFlag = 0;
      for (int pos = 0; pos < 59; pos++)
      {
         bool s = (processingBuffer >> pos) & 1;

         // Update the parity bits. First: Reset when minute, hour or date starts.
         if (pos == 21 || pos == 29 || pos == 36)
         {
            flags.parityFlag = 0;
         }
         // save the parity when the corresponding segment ends
         if (pos == 28)
         {
            flags.parityMin = flags.parityFlag;
         };
         if (pos == 35)
         {
            flags.parityHour = flags.parityFlag;
         };
         if (pos == 58)
         {
            flags.parityDate = flags.parityFlag;
         };
         // When we received a 1, toggle the parity flag
         if (s == 1)
         {
            flags.parityFlag = flags.parityFlag ^ 1;
         }
      }
   }

   bool processBuffer(void)
   {

      /////  Start interaction with interrupt driven loop  /////

      // Copy filled buffer and timestamp from interrupt driven loop
      processingBuffer = filledBuffer;
      processingTimestamp = filledTimestamp;
      // Indicate that there is no filled, unprocessed buffer anymore
      FilledBufferAvailable = false;

      /////  End interaction with interrupt driven loop   /////

      //  Calculate parities for checking buffer
      calculateBufferParities();
      tmElements_t time;

      struct DCF77Buffer *rx_buffer;
      rx_buffer = (struct DCF77Buffer *)(unsigned long long)&processingBuffer;

      // Check parities
      if (flags.parityMin == rx_buffer->P1 &&
          flags.parityHour == rx_buffer->P2 &&
          flags.parityDate == rx_buffer->P3 &&
          rx_buffer->CEST != rx_buffer->CET)
      {
         // convert the received buffer into time
         time.Second = 0;
         time.Minute = rx_buffer->Min - ((rx_buffer->Min / 16) * 6);
         time.Hour = rx_buffer->Hour - ((rx_buffer->Hour / 16) * 6);
         time.Day = rx_buffer->Day - ((rx_buffer->Day / 16) * 6);
         time.Month = rx_buffer->Month - ((rx_buffer->Month / 16) * 6);
         time.Year = 2000 + rx_buffer->Year - ((rx_buffer->Year / 16) * 6) - 1970;
         latestupdatedTime = makeTime(time);
         CEST = rx_buffer->CEST;
         // Parity correct
         return true;
      }
      else
      {
         // Parity incorrect
         return false;
      }
   }

   void appendSignal(unsigned char signal)
   {
      runningBuffer = runningBuffer | ((unsigned long long)signal << bufferPosition);
      bufferPosition++;
      if (bufferPosition > 59)
      {
         // Buffer is full before at end of time-sequence
         // this may be due to noise giving additional peaks
         DEBUG_PRINTLN("EoB");
         finalizeBuffer();
      }
   }

   DCF77(int DCF77Pin, int DCFinterrupt, bool OnRisingFlank = true)
   {

      dCF77Pin = DCF77Pin;
      dCFinterrupt = DCFinterrupt;
      pulseStart = OnRisingFlank ? HIGH : LOW;

      pinMode(dCF77Pin, INPUT_PULLUP);
      initialize();
   }

   time_t getTime(void)
   {
      if (!receivedTimeUpdate())
      {
         return (0);
      }
      else
      {
         // Send out time, taking into account the difference between when the DCF time was received and the current time
         time_t currentTime = latestupdatedTime + (now() - processingTimestamp);
         return (currentTime);
      }
   }

   time_t getUTCTime(void)
   {
      if (!receivedTimeUpdate())
      {
         return (0);
      }
      else
      {
         // Send out time UTC time
         int UTCTimeDifference = (CEST ? 2 : 1) * SECS_PER_HOUR;
         time_t currentTime = latestupdatedTime - UTCTimeDifference + (now() - processingTimestamp);
         return (currentTime);
      }
   }

   void Start(void)
   {
      attachInterrupt(digitalPinToInterrupt(dCF77Pin), int0handler, CHANGE);
      // attachInterrupt(dCFinterrupt, int0handler, CHANGE);
   }

   void Stop(void)
   {
      detachInterrupt(digitalPinToInterrupt(dCF77Pin));
   }

   bool m_bInitialized;
   int dCF77Pin;
   int dCFinterrupt;
   byte pulseStart;

   // DCF77 and internal timestamps
   time_t previousUpdatedTime;
   time_t latestupdatedTime;
   time_t processingTimestamp;
   time_t previousProcessingTimestamp;
   unsigned char CEST;
};

DCF77 *g_pDcf77Instance = nullptr;

static void IRAM_ATTR int0handler()
{
   int flankTime = millis();
   byte sensorValue = digitalRead(g_pDcf77Instance->dCF77Pin);

   // If flank is detected quickly after previous flank up
   // this will be an incorrect pulse that we shall reject
   if ((flankTime - g_pDcf77Instance->PreviousLeadingEdge) < DCFRejectionTime)
   {
      DEBUG_PRINTLN("rCT");
      return;
   }

   // If the detected pulse is too short it will be an
   // incorrect pulse that we shall reject as well
   if ((flankTime - g_pDcf77Instance->leadingEdge) < DCFRejectPulseWidth)
   {
      DEBUG_PRINTLN("rPW");
      return;
   }

   if (sensorValue == g_pDcf77Instance->pulseStart)
   {
      if (!g_pDcf77Instance->Up)
      {
         // Flank up
         g_pDcf77Instance->leadingEdge = flankTime;
         g_pDcf77Instance->Up = true;
      }
   }
   else
   {
      if (g_pDcf77Instance->Up)
      {
         // Flank down
         g_pDcf77Instance->trailingEdge = flankTime;
         int difference = g_pDcf77Instance->trailingEdge - g_pDcf77Instance->leadingEdge;

         if ((g_pDcf77Instance->leadingEdge - g_pDcf77Instance->PreviousLeadingEdge) > DCFSyncTime)
         {
            g_pDcf77Instance->finalizeBuffer();
         }
         g_pDcf77Instance->PreviousLeadingEdge = g_pDcf77Instance->leadingEdge;
         // Distinguish between long and short pulses
         if (difference < DCFSplitTime)
         {
            g_pDcf77Instance->appendSignal(0);
         }
         else
         {
            g_pDcf77Instance->appendSignal(1);
         }
         g_pDcf77Instance->Up = false;
      }
   }
}

class UsermodDcf77 : public Usermod
{
public:
   UsermodDcf77() : m_pDcf(nullptr),
                    m_uiPeriodMs(2000),
                    m_ulTimLastCall(0),
                    m_iDcfPin(-1)
   {
   }

   virtual void loop()
   {
      unsigned long uiTimAct = millis();

      if (uiTimAct > (m_ulTimLastCall + m_uiPeriodMs))
      {

         // check if DCF object was not yet created and a valid GPIO pin is set
         if (m_pDcf == nullptr)
         {
            if (m_iDcfPin > 0)
            {
               DEBUG_PRINT("Found valid GPIO setting for DCF77 Signal(t) using GPIO: ");
               DEBUG_PRINTLN(m_iDcfPin);
               m_pDcf = new DCF77(m_iDcfPin, m_iDcfPin);
               g_pDcf77Instance = m_pDcf;
               m_pDcf->Start();
            }
            else
            {
               DEBUG_PRINTLN("There is no valid GPIO setting for DCF77 Signal(t) - Try \"15\" for GPIO15 (D8).");
            }
         }
         else
         {
            time_t DCFtime = m_pDcf->getTime(); // Check if new DCF77 time is available
            if (DCFtime != 0)
            {
               DEBUG_PRINT("Time was updated by DCF77: ");
               printTime(DCFtime);
               setTime(DCFtime);
            }
            else
            {
               DEBUG_PRINTLN("Waiting for DCF77 time update.");
               // printTime(localTime);
            }
         }
         m_ulTimLastCall = uiTimAct;
      }
   }

   virtual void setup()
   {
   }

   virtual void addToJsonState(JsonObject &obj)
   {
   }

   virtual void readFromJsonState(JsonObject &obj)
   {
   }

   virtual void addToConfig(JsonObject &obj)
   {
      JsonObject jsConfig = obj.createNestedObject("Dcf77");
      jsConfig["GPIO"] = m_iDcfPin;
   }

   virtual bool readFromConfig(JsonObject &obj)
   {
      JsonObject jsConfig = obj["Dcf77"];

      bool bRetval = false;

      if (!jsConfig.isNull())
      {
         bRetval = getJsonValue(jsConfig["GPIO"], m_iDcfPin);
      }
      return bRetval;
   }

   virtual void appendConfigData() override
   {
      oappend(SET_F("addInfo('Dcf77:GPIO',1,' t-signal of DCF77 module. Use 15 for GPIO15 (D8-Pin on D1-mini).');"));
   }

   /*
    * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
    * This could be used in the future for the system to determine whether your usermod is installed.
    */
   uint16_t getId()
   {
      return USERMOD_ID_DCF77;
   }

private:
   DCF77 *m_pDcf;
   unsigned long m_uiPeriodMs;
   unsigned long m_ulTimLastCall;
   int8_t m_iDcfPin;
};

#endif
