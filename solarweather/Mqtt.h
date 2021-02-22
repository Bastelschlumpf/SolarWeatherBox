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
  * @file Mqtt.h
  *
  * Communication with an MQTT server.
  */


#include <PubSubClient.h>

#define topic_deep_sleep       "/DeepSleep"          //!< Deep sleep on/off
#define topic_send_every       "/SendEverySec"       //!< mqtt sending interval

#define topic_temperature      "/BME280/Temperature" //!< Temperature
#define topic_humidity         "/BME280/Humidity"    //!< Humidity
#define topic_pressure         "/BME280/Pressure"    //!< Pressure

#define topic_voltage          "/Voltage"            //!< Power supply voltage
#define topic_mAh              "/mAh"                //!< Power consumption
#define topic_alive            "/Alive"              //!< Alive time in sec
#define topic_rssi             "/RSSI"               //!< Wifi conection quality

#define topic_conn_error_count "/ConnErrorCount"     //!< Connection error Count
#define topic_send_error_count "/SendErrorCount"     //!< mqtt sending error count

/**
  * MQTT client for sending the collected data to a MQTT server
  */
class MyMqtt : protected PubSubClient
{
protected:
   static MyOptions *g_myOptions;   //!< Static option pointer for the callback function.

public:
   static void mqttCallback(char* topic, byte* payload, unsigned int len);
   
protected:
   MyOptions &myOptions;            //!< Reference to the options. 
   MyData    &myData;               //!< Reference to the data.
   bool       publishInProgress;    //!< Are we publishing right now.

protected:
   bool mySubscribe(String subTopic);
   bool myPublish(String subTopic, String value);

public:
   MyMqtt(Client &client, MyOptions &options, MyData &data);
   ~MyMqtt();
   
   bool begin();
   void handleClient();
   
   bool waitingForMqtt();
};

/* ******************************************** */

/** Constructor/Destructor */
MyMqtt::MyMqtt(Client &client, MyOptions &options, MyData &data)
   : PubSubClient(client)
   , myOptions(options)
   , myData(data)
   , publishInProgress(false)
{
   g_myOptions = &options;
}
MyMqtt::~MyMqtt()
{
   g_myOptions = NULL;
}

/** Helper function to subscrbe on mqtt 
 *  It put the mqttName and id from options before the topic.
*/
bool MyMqtt::mySubscribe(String subTopic)
{
   String topic;

   topic = myOptions.mqttName + F("/") + myOptions.mqttId + subTopic;
   MyDbg((String) F("MyMqtt::subscribe: [") + topic + F("]"), true);
   return PubSubClient::subscribe(topic.c_str());
}

/** Helper function to publish on mqtt 
 *  It put the mqttName from optione before the topic.
*/
bool MyMqtt::myPublish(String subTopic, String value)
{
   bool ret = false;

   if (value.length() > 0) {
      String topic;

      topic = myOptions.mqttName + F("/") + myOptions.mqttId + subTopic;
      MyDbg((String) F("MyMqtt::publish: [") + topic + F("]=[") + value + F("]"), true);
      ret = PubSubClient::publish(topic.c_str(), value.c_str(), true);
      if (!ret) myData.rtcData.mqttSendErrorCount++;
   }
   return ret;
}

/** Check if we have to wait for sending mqtt data. */
bool MyMqtt::waitingForMqtt()
{
   if (publishInProgress) {
      return true;
   }
   return secondsElapsed(myData.rtcData.lastMqttPublishSec, myOptions.mqttSendEverySec);
}

/** Sets the MQTT server settings */
bool MyMqtt::begin()
{
   MyDbg(F("MQTT:begin"), true);
   MyDbg((String) "MQTT:setServer(" + myOptions.mqttServer + ", " + myOptions.mqttPort + ")", true);
   PubSubClient::setServer(myOptions.mqttServer.c_str(), myOptions.mqttPort);
   PubSubClient::setCallback(mqttCallback);
   return true;
}

/** Connect To the MQTT server and send the data when the time is right. */
void MyMqtt::handleClient()
{
   bool send = secondsElapsed(myData.rtcData.lastMqttPublishSec, myOptions.mqttSendEverySec);
   if (send && !publishInProgress) {
      publishInProgress = true;
      if (!PubSubClient::connected()) {
         for (int i = 0; !PubSubClient::connected() && i < 25; i++) {  
            MyDbg((String) "Attempting MQTT connection..." + " [" + myOptions.mqttName + "][" + myOptions.mqttUser + "][" + myOptions.mqttPassword + "]", true);
            if (PubSubClient::connect(myOptions.mqttName.c_str(), myOptions.mqttUser.c_str(), myOptions.mqttPassword.c_str())) {  
               // mySubscribe(topic_deep_sleep);
               MyDbg(F(" connected"), true);
            } else {  
               MyDbg((String) F("   Mqtt failed, rc = ") + String(PubSubClient::state()), true);
               MyDbg(F(" Try again in 1 second"), true);
               MyDelay(1000);
               MyDbg(F("."), true, false);
            }  
         }  
      }
      if (!PubSubClient::connected()) {
         myData.rtcData.mqttConnErrorCount++;
      } else {
         MyDbg(F("Attempting MQTT publishing"), true);
         myPublish(topic_temperature,      String(myData.temperature));
         myPublish(topic_humidity,         String(myData.humidity));
         myPublish(topic_pressure,         String(myData.pressure));
         myPublish(topic_voltage,          String(myData.voltage, 2));
         myPublish(topic_mAh,              String(myData.getPowerConsumption()));
         myPublish(topic_alive,            formatInterval(myData.getActiveTimeSec()));
         myPublish(topic_rssi,             String(WiFi.RSSI()));
         myPublish(topic_conn_error_count, String(myData.rtcData.mqttConnErrorCount));
         myPublish(topic_send_error_count, String(myData.rtcData.mqttSendErrorCount));
         myData.rtcData.mqttSendCount++;
         MyDbg(F("mqtt published"), true);
         MyDelay(5000);
      }
      // Set time even on error
      myData.rtcData.lastMqttPublishSec = myData.getActiveTimeSec();
      publishInProgress = false;
   }
}

MyOptions *MyMqtt::g_myOptions = NULL;

/** Static function for MQTT callback on registered topics. */
void MyMqtt::mqttCallback(char* topic, byte* payload, unsigned int len) 
{
   if (topic == NULL || payload == NULL || len <= 0 || len > 200) {
      return;
   }

   String strTopic = String((char*)topic);

   payload[len] = '\0';
   MyDbg((String) F("Message arrived [") + strTopic + F("]:[ "), true);
   if (len) MyDbg((char *) payload, true);
   MyDbg(F("]"), true);

   if (MyMqtt::g_myOptions) {
      if (strTopic == g_myOptions->mqttName + topic_deep_sleep) {
         g_myOptions->isDeepSleepEnabled = atoi((char *) payload);
         MyDbg(strTopic + g_myOptions->isDeepSleepEnabled ? F(" - On") : F(" - Off"), true);
      }
      if (strTopic == g_myOptions->mqttName + topic_send_every) {
         g_myOptions->mqttSendEverySec = atoi((char *) payload);
         MyDbg(strTopic + " - " + String(g_myOptions->mqttSendEverySec), true);
      }
   }
}
