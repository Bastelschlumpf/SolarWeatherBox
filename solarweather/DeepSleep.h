/*
   Copyright (C) 2021 SFini

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
*/
/**
  * @file DeepSleep.h
  * 
  * DeepSleep functions.
  */

#define NO_DEEP_SLEEP_STARTUP_TIME 120     //!< No deep sleep for the first two minutes.
#define MAX_DEEP_SLEEP_TIME_SEC    60 * 60 //!< Maximum deep sleep time (60 minutes)
#define DEEP_SLEEP_CORRECT         1.09    //!< Correction try for the deep sleep inaccuracy


/**
  * Class to read/save a deepsleep counter and start the deepsleep mode
  * if the voltage is too low.
  */
class MyDeepSleep
{
protected:
   MyOptions &myOptions;     //!< Reference to the options
   MyData    &myData;        //!< Reference to the data
   
public:
   MyDeepSleep(MyOptions &options, MyData &data);

   bool begin();
   
   bool haveToSleep();
   void updateTimeToSleep();
   void sleep();
};

/* ******************************************** */

/** Constructor */
MyDeepSleep::MyDeepSleep(MyOptions &options, MyData &data)
   : myOptions(options)
   , myData(data)
{
}

/**
  * Read the deepsleep counter from the RTC memory.
  * Use a simple random value variable to identify if the counter is still 
  * initialized
  */
bool MyDeepSleep::begin()
{
   MyDbg(F("MyDeepSleep::begin"));
   
   MyData::RtcData rtcData;

   ESP.rtcUserMemoryRead(0, (uint32_t *) &rtcData, sizeof(MyData::RtcData));
   if (!rtcData.isValid()) {
      MyDbg(F("RtcData invalid (power on?)"));
   } else {
      MyDbg(F("RtcData read"));
      myData.rtcData = rtcData;
   }
   return true;
}

/** Calculate the time needed for the next deep sleep. */
void MyDeepSleep::updateTimeToSleep()
{
   myData.secondsToDeepSleep = -1;
   if (myOptions.isDeepSleepEnabled) {
      long activeTimeSec = myData.getActiveTimeSec() - myData.awakeTimeOffsetSec;
      
      myData.secondsToDeepSleep = max(myOptions.activeTimeSec - activeTimeSec, NO_DEEP_SLEEP_STARTUP_TIME - myData.getActiveTimeSumSec());
   }
}

/** Check if the configured time has elapsed and the voltage is too low then go into deep sleep. */
bool MyDeepSleep::haveToSleep()
{
   if (myData.rtcData.deepSleepTimeRestSec > 0) {
      return true;
   } else {
      long activeTimeSec = myData.getActiveTimeSec() - myData.awakeTimeOffsetSec;
       
      return (myOptions.isDeepSleepEnabled &&
              myData.getActiveTimeSumSec() > NO_DEEP_SLEEP_STARTUP_TIME &&
              activeTimeSec                >= myOptions.activeTimeSec);
   }
}

/**
  * Entering the DeepSleep mode. Be sure we have connected the RST pin to the D0 pin for wakeup.
  * If the deep sleep mode time is above the maximum then we do it stepwise.
  */
void MyDeepSleep::sleep()
{
   long deepSleepTimeSec = myOptions.deepSleepTimeSec;

   if (myData.rtcData.deepSleepTimeRestSec > 0) {
      deepSleepTimeSec = myData.rtcData.deepSleepTimeRestSec;
      if (deepSleepTimeSec < MAX_DEEP_SLEEP_TIME_SEC) {
         myData.rtcData.deepSleepTimeRestSec = 0;
      }
   }
   if (deepSleepTimeSec >= MAX_DEEP_SLEEP_TIME_SEC) {
      myData.rtcData.deepSleepTimeRestSec = deepSleepTimeSec - MAX_DEEP_SLEEP_TIME_SEC;
      if (myData.rtcData.deepSleepTimeRestSec < 0) {
         myData.rtcData.deepSleepTimeRestSec = 0;
      }
      deepSleepTimeSec = MAX_DEEP_SLEEP_TIME_SEC;
   }

   myData.rtcData.activeTimeSumSec    += myData.getActiveTimeSec();
   myData.rtcData.deepSleepTimeSumSec += deepSleepTimeSec;
   myData.rtcData.setCRC();
   ESP.rtcUserMemoryWrite(0, (uint32_t *) &myData.rtcData, sizeof(MyData::RtcData));

   WiFi.disconnect();
   WiFi.mode(WIFI_OFF);
   WiFi.forceSleepBegin();
   yield();
   
   MyDbg((String) F("Entering deep sleep for: ") + String(deepSleepTimeSec) + F(" sec"));
   delay(1000);
   ESP.deepSleep(deepSleepTimeSec * DEEP_SLEEP_CORRECT * 1000000ULL);
}
