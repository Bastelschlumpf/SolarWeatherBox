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
  * @file Data.h
  * 
  * Class with all the global runtime data.
  */


/**
  * Helper class to store all the global determined data in one place.
  */
class MyData
{
public:
   /**
     * Data to store in the RTC memory
     */
   class RtcData {
   public:
      long activeTimeSumSec;       //!< Time on power with current millis..
      long deepSleepTimeSumSec;    //!< Time in deep sleep mode. 
      long deepSleepStartSec;      //!< Timestamp of the last deep sleep start.
                 
      long lastBme280ReadSec;      //!< Timestamp of the last BME280 read.
      long lastMqttPublishSec;     //!< Timestamp from the last send.

      long mqttConnErrorCount;     //!< How many time the mqtt connection to the server fails.
      long mqttSendCount;          //!< How many time the mqtt data successfully sent.
      long mqttSendErrorCount;     //!< How many time the mqtt sending failed.
                 
      long crcValue;               //!< CRC of the RtcData

   public:
      RtcData();

      bool isValid();
      void setCRC();
      long getCRC();
   } rtcData;                  //!< Data to store in the RTC memory.

   String status;              //!< Status information
   String restartInfo;         //!< Information on restart
   bool   isOtaActive;         //!< Is OverTheAir update active?
   bool   isLowPower;          //!< Is the power below min voltage?
   
   long   secondsToDeepSleep;  //!< Time until next deepsleep. -1 = disabled
   long   awakeTimeOffsetSec;  //!< Awake time offset for SaveSettings.

   double voltage;             //!< Current supply voltage
   double temperature;         //!< Current BME280 temperature
   double humidity;            //!< Current BME280 humidity
   double pressure;            //!< Current BME280 pressure

   String softAPIP;            //!< registered ip of the access point
   String softAPmacAddress;    //!< module mac address
   String stationIP;           //!< registered station ip
   
   String signalQuality;       //!< Quality of the signal
   String batteryLevel;        //!< Battery level of the sim808 module
   String batteryVolt;         //!< Battery volt of the sim808 module
   
   StringList consoleCmds;     //!< open commands to send to the sim808 module
   StringList logInfos;        //!< received sim808 answers or other logs

public:
   MyData();

   long   getActiveTimeSec();
   long   getActiveTimeSumSec();
   long   getDeepSleepTimeSumSec();

   double getPowerConsumption();
};

/* ******************************************** */

MyData::RtcData::RtcData()
   : activeTimeSumSec(0)
   , deepSleepTimeSumSec(0)
   , deepSleepStartSec(0)
   , lastBme280ReadSec(0)
   , lastMqttPublishSec(0)
   , mqttConnErrorCount(0)
   , mqttSendCount(0)
   , mqttSendErrorCount(0)
{
   crcValue = getCRC();
}

/** Does the CRC fit s the content */
bool MyData::RtcData::isValid()
{
   return getCRC() == crcValue;
}

/** Creates the CRC of all the data and save it in the class. */
void MyData::RtcData::setCRC()
{
   crcValue = getCRC();
}

/** Creates a CRC of all the member variables. */
long MyData::RtcData::getCRC()
{
   long crc = 0;

   crc = crc32(crc, (unsigned char *) &activeTimeSumSec,    sizeof(long));
   crc = crc32(crc, (unsigned char *) &deepSleepTimeSumSec, sizeof(long));
   crc = crc32(crc, (unsigned char *) &deepSleepStartSec,   sizeof(long));
   crc = crc32(crc, (unsigned char *) &lastBme280ReadSec,   sizeof(long));
   crc = crc32(crc, (unsigned char *) &lastMqttPublishSec,  sizeof(long));
   crc = crc32(crc, (unsigned char *) &mqttConnErrorCount,  sizeof(long));
   crc = crc32(crc, (unsigned char *) &mqttSendCount,       sizeof(long));
   crc = crc32(crc, (unsigned char *) &mqttConnErrorCount,  sizeof(long));
   
   return crc;
}

/** Constructor */
MyData::MyData()
   : isOtaActive(false)
   , isLowPower(false)
   , secondsToDeepSleep(-1)
   , awakeTimeOffsetSec(0)
   , voltage(0.0)
   , temperature(0.0)
   , humidity(0.0)
   , pressure(0.0)
{
}

/** Returns the seconds since power up (not since last deep sleep). */
long MyData::getActiveTimeSec()
{
   return millis() / 1000;
}

/** Return all the active over all deep sleeps plus the current active time. */
long MyData::getActiveTimeSumSec()
{
   return rtcData.activeTimeSumSec + getActiveTimeSec();
}

/** Return all the deep sleep time. */
long MyData::getDeepSleepTimeSumSec()
{
   return rtcData.deepSleepTimeSumSec;
}

/** Calculates the power consumption from power on.
  * In mA/h
  */
double MyData::getPowerConsumption()
{
   return (POWER_CONSUMPTION_ACTIVE     * getActiveTimeSumSec() +
           POWER_CONSUMPTION_DEEP_SLEEP * getDeepSleepTimeSumSec()) / 3600.0;
}
