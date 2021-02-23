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
  * @file tracker.ino 
  *
  * Main file with setup() and loop() functions
  */


#include <SoftwareSerial.h>
#include <ArduinoOTA.h>
#include <FS.h>

#include "Config.h"
#define USE_CONFIG_OVERRIDE //!< Switch to use ConfigOverride
#ifdef USE_CONFIG_OVERRIDE
  #include "ConfigOverride.h"
#endif

#include "Utils.h"
#include "StringList.h"
#include "Options.h"
#include "Data.h"
#include "Voltage.h"
#include "DeepSleep.h"
#include "WebServer.h"
#include "Mqtt.h"
#include "BME280.h"


MyOptions   myOptions;                       //!< The global options.
MyData      myData;                          //!< The global collected data.
MyVoltage   myVoltage   (myOptions, myData); //!< Helper class for deep sleeps.
MyDeepSleep myDeepSleep (myOptions, myData); //!< Helper class for deep sleeps.
MyWebServer myWebServer (myOptions, myData); //!< The Webserver
MyBME280    myBME280    (myOptions, myData); //!< Helper class for the BME280 sensor communication.

MyMqtt      myMqtt(MyWebServer::server.wifiClient(), myOptions, myData); 

bool        isStarting  = false;             //!< Are we in a starting process?
bool        isStopping  = false;             //!< Are we in a stopping process?

/**
 * ***** IMPORTANT *****
 * It seems that the yield function of ESP does not feed the watch dog timer.
 * So we overwrite the yield() function here. If not the program crashed on some waits.
 * ***** IMPORTANT *****
 */
 /*
void yield(void)
{
   ESP.wdtFeed();
}*/

/** Overwritten Debug Function 
  * It logs all the debug calls to the console string-list
  * And call a refresh of the webserver for not blocking the system.
  */
void myDebugInfo(String info, bool fromWebserver, bool newline)
{
   static bool lastNewLine = true;
   
   if (newline || lastNewLine != newline) {
      String secs = String(myData.getActiveTimeSec()) + F(": ");

      myData.logInfos.addTail(secs + info);
      if (!lastNewLine) {
         Serial.println("");
      }
      Serial.println(secs + info);
   } else {
      String tmp = myData.logInfos.removeTail();

      Serial.print(tmp + info);
      myData.logInfos.addTail(tmp + info);
   }
   lastNewLine = newline;

   if (!fromWebserver) {
      myWebServer.handleClient();   
      myWebServer.handleClient();   
      myWebServer.handleClient();   
   } 
   delay(1);
   yield();
}

/** Overwritten delay loop for refreshing the webserver on waiting processes. */
void myDelayLoop()
{
   myWebServer.handleClient();   
   myWebServer.handleClient();   
   myWebServer.handleClient(); 
   delay(1);
   yield();
}

/** Main setup function. This is also called after every deep sleep. 
  * Do the initialization of every sub-component. */
void setup() 
{
   Serial.begin(115200); 
   delay(1000);

   MyDbg(F("Start SolarWeather ..."));

   SPIFFS.begin();
   myOptions.load();

   // Back to deep sleep?
   myDeepSleep.begin();
   if (myDeepSleep.haveToSleep()) {
      myDeepSleep.sleep();
   } else { // no deep sleep!
      myVoltage.begin();
      myWebServer.begin();
      myMqtt.begin();
      myBME280.begin();
   }
}

/** Main loop function.
  * Read the power supply voltage.
  * Checks for OTA activities.
  * Starts the deep sleep mode if needed.
  */
void loop() 
{
   myVoltage.readVoltage();
   myBME280.readValues();

   myWebServer.handleClient();
   
   if (myData.isOtaActive) {
      ArduinoOTA.handle();    
   }

   if (myOptions.isMqttEnabled) {
      myMqtt.handleClient();
   }

   if (!myMqtt.waitingForMqtt()) {
      if (myDeepSleep.haveToSleep()) {
         WiFi.disconnect();
         WiFi.mode(WIFI_OFF);
         yield();
         myDeepSleep.sleep();
      }
   }

   yield();
   delay(10); 
}
