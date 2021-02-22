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
  * @file Options.h
  *
  * Configuration data with load and save to the SPIFFS.
  */

#define OPTION_FILE_NAME "/options.txt" //!< Option file name.

/** 
  * Class with the complete configuration data of the programm.
  * It can load and save the data in a ini file format to the SPIFFS
  * as key value pairs 'key=value' line by line.
  */
class MyOptions
{
public:
   bool   connectWifiAP;             //!< Should we connect to wifi
   String wifiAP;                    //!< WiFi AP name.
   String wifiPassword;              //!< WiFi AP password.
   bool   isDebugActive;             //!< Is detailed debugging enabled?
   long   bme280CheckIntervalSec;    //!< Time interval to read the temp, hum and pressure.
   bool   isDeepSleepEnabled;        //!< Should the system go into deepsleep if needed.
   long   activeTimeSec;             //!< Maximum alive time after deepsleep.
   long   deepSleepTimeSec;          //!< Time to stay in deep sleep (without check interrupts)
   bool   isMqttEnabled;             //!< Should the system connect to a MQTT server?
   String mqttName;                  //!< MQTT server name.
   String mqttId;                    //!< MQTT ID.
   String mqttServer;                //!< MQTT server url.
   long   mqttPort;                  //!< MQTT server port.
   String mqttUser;                  //!< MQTT user.
   String mqttPassword;              //!< MQTT password.
   long   mqttSendEverySec;          //!< Send data interval to MQTT server.

public:
   MyOptions();

   bool load();
   bool save();
};

/* ******************************************** */

MyOptions::MyOptions()
   : isDebugActive(false)
   , wifiAP(WIFI_SID)
   , connectWifiAP(true)
   , wifiPassword(WIFI_PW)
   , bme280CheckIntervalSec(60) // 1 Min
   , isDeepSleepEnabled(false)
   , activeTimeSec(60)          //  1 Min
   , deepSleepTimeSec(900)      // 15 Min
   , isMqttEnabled(false)
   , mqttName(MQTT_NAME)
   , mqttId(MQTT_ID)
   , mqttServer(MQTT_SERVER)
   , mqttPort(MQTT_PORT)
   , mqttUser(MQTT_USER)
   , mqttPassword(MQTT_PASSWORD)
   , mqttSendEverySec(900)      //  15 Min
{
}

/** Load the key-value pairs from the option file into the option values. */
bool MyOptions::load()
{
   bool ret  = false;
   File file = SPIFFS.open(OPTION_FILE_NAME, "r");

   if (!file) {
      MyDbg(F("Failed to read options file"));
   } else {
      ret = true;
      while (file.available() && ret) {
         String line = file.readStringUntil('\n');
         int    idx  = line.indexOf('=');

         if (idx == -1) {
            MyDbg((String) F("Wrong option entry: ") + line);
            ret = false;
         } else {
            String key    = line.substring(0, idx);
            String value  = line.substring(idx + 1);
            long   lValue = atol(value.c_str());
            double fValue = atof(value.c_str());

            value.replace("\r", "");
            value.replace("\n", "");
            MyDbg((String) F("Load option '") + key + F("=") + value + F("'"));

            if (key == F("isDebugActive")) {
               isDebugActive = lValue;
            } else if (key == F("wifiAP")) {
               wifiAP = value;
            } else if (key == F("connectWifiAP")) {
               connectWifiAP = lValue;
            } else if (key == F("wifiPassword")) {
               wifiPassword = value;
            } else if (key == F("bme280CheckIntervalSec")) {
               bme280CheckIntervalSec = lValue;
            } else if (key == F("isDeepSleepEnabled")) {
               isDeepSleepEnabled = lValue;
            } else if (key == F("activeTimeSec")) {
               activeTimeSec = lValue;
            } else if (key == F("deepSleepTimeSec")) {
               deepSleepTimeSec = lValue;
            } else if (key == F("isMqttEnabled")) {
               isMqttEnabled = lValue;
            } else if (key == F("mqttName")) {
               mqttName = value;
            } else if (key == F("mqttId")) {
               mqttId = value;
            } else if (key == F("mqttServer")) {
               mqttServer = value;
            } else if (key == F("mqttPort")) {
               mqttPort = lValue;
            } else if (key == F("mqttUser")) {
               mqttUser = value;
            } else if (key == F("mqttPassword")) {
               mqttPassword = value;
            } else if (key == F("mqttSendEverySec")) {
               mqttSendEverySec = lValue;
            } else {
               MyDbg((String) F("Wrong option entry: ") + line);
               ret = false;
            }
         }
      }
      file.close();
   }
   if (ret) {
      MyDbg(F("Settings loaded"));
   }
   return ret;
}

/** Save all the options as key-value pair to the option file. */
bool MyOptions::save()
{
  File file = SPIFFS.open(OPTION_FILE_NAME, "w+");

  if (!file) {
     MyDbg("Failed to write options file");
  } else {
     file.println((String) F("isDebugActive=")          + String(isDebugActive));
     file.println((String) F("connectWifiAP=")          + String(connectWifiAP));
     file.println((String) F("wifiAP=")                 + wifiAP);
     file.println((String) F("wifiPassword=")           + wifiPassword);
     file.println((String) F("bme280CheckIntervalSec=") + String(bme280CheckIntervalSec));
     file.println((String) F("isDeepSleepEnabled=")     + String(isDeepSleepEnabled));
     file.println((String) F("activeTimeSec=")          + String(activeTimeSec));
     file.println((String) F("deepSleepTimeSec=")       + String(deepSleepTimeSec));
     file.println((String) F("isMqttEnabled=")          + String(isMqttEnabled));
     file.println((String) F("mqttName=")               + mqttName);
     file.println((String) F("mqttId=")                 + mqttId);
     file.println((String) F("mqttServer=")             + mqttServer);
     file.println((String) F("mqttPort=")               + String(mqttPort));
     file.println((String) F("mqttUser=")               + mqttUser);
     file.println((String) F("mqttPassword=")           + mqttPassword);
     file.println((String) F("mqttSendEverySec=")       + String(mqttSendEverySec));
     file.close();
     MyDbg(F("Settings saved"));
     return true;
  }
  return false;
}
