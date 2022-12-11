// include Arduino library to use Arduino function in cpp files
#include <Arduino.h>
// include Json library
#include <ArduinoJson.h>
// include MQTT library (https://pubsubclient.knolleary.net/api)
#include <PubSubClient.h>

#include "secrets.h"

// private functions
void mqtt_subscribe(String uuid, String command);

const char* mqtt_url = MQTT_URL;
const int mqtt_port = MQTT_PORT;

PubSubClient mqtt_client;
int mqtt_retries = 0;

void mqtt_init(Client& wifi_client, std::function<void (char *, uint8_t *, unsigned int)> callback) {
  mqtt_client.setServer(mqtt_url, mqtt_port);
  mqtt_client.setClient(wifi_client);
  mqtt_client.setCallback(callback);
}

void mqtt_connect(String uuid) { 
  while (!mqtt_client.connected()) {
    Serial.println("mqtt_connect - attempting MQTT connection...");
    mqtt_client.setBufferSize(4096);
    
    // here you can use the version with `connect(const char* id, const char* user, const char* pass)` with authentication
    const char* id_client = uuid.c_str();
    Serial.print("mqtt_connect - connecting to MQTT with client id = ");
    Serial.println(id_client);

    bool connected = false;
    # if MQTT_AUTH==true
      Serial.println("mqtt_connect - connecting to MQTT with authentication");
      const char* mqtt_username = MQTT_USERNAME; 
      const char* mqtt_password = MQTT_PASSWORD;
      connected = mqtt_client.connect(id_client, mqtt_username, mqtt_password);
    # else
      Serial.println("mqtt_connect - connecting to MQTT without authentication");
      connected = mqtt_client.connect(id_client)
    # endif

    if (connected) {
      Serial.print("mqtt_connect - connected and subscribing with uuid: ");
      Serial.println(uuid);
      mqtt_retries = 0;
      // subscribe
      mqtt_subscribe(uuid, "values");
    } else {
      Serial.print("mqtt_connect - failed, rc=");
      Serial.print(mqtt_client.state());
      mqtt_retries++;
      Serial.println(" - try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    // after 100 retries (100 * 5 = 500 seconds) without success, reboot this device
    if (mqtt_retries > 100) {
      ESP.restart();
    }
  }
}

void mqtt_subscribe(String uuid, String command) {
  Serial.println("mqtt_subscribe - creating topic based on command...");
  String topic = "devices/" + uuid + "/" + command;
  Serial.print("mqtt_subscribe - topic = ");
  Serial.println(topic);
  mqtt_client.subscribe(topic.c_str());
}