// include the WiFi library and HTTPClient
#include <WiFi.h>
#include <HTTPClient.h>
// include json library (https://github.com/bblanchon/ArduinoJson)
#include <ArduinoJson.h>
// include MQTT library
#include <PubSubClient.h>
// eeprom lib has been deprecated for esp32, the recommended way is to use Preferences
#include <Preferences.h>

// must be the first, before any internal include
#include "secrets.h"

// include all local files
#include "wifi_handler.h"
#include "mqtt_handler.h"
#include "registration.h"
#include "storage.h"
#include "ir_lg.h"

// private functions
void mqtt_callback(char* topic, byte* payload, unsigned int length);

String saved_uuid;

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("mqtt_callback - called");
  ir_send_command(topic, payload, length);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("setup - starting...");
  
  // 0. configure hardware
  //    disable ESP builtin LED
  //    but not all ESP boards have this variable defined, so I should check the existance of `LED_BUILTIN`.
  #ifdef LED_BUILTIN
    // disable ESP32 Devkit-C built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
  #endif

  // 1. prepare wifi_client
  //    If SSL is enabled, add root ca to wifi_client to be used for secure connections
  //    This root ca will be used by all network components like mqtt and htpp
  # if SSL==true
    Serial.println("setup - running with SSL enabled");
    wifi_init_ca();
  # else 
    Serial.println("setup - running WITHOUT SSL");
  # endif

  // 2. init MQTT client
  //    init mqtt_client with wifi_client (previously configured)
  Serial.println("setup - init mqtt...");
  mqtt_init(wifi_client, mqtt_callback);

  // 3. connect to wifi
  Serial.println("setup - connect wifi...");
  wifi_connect();

  // 4. register to the server
  Serial.println("setup - registering this device...");
  String features = "[";
  features += "{\"type\": \"controller\",\"name\": \"ac-lg\",\"enable\": true,\"order\": 1,\"unit\": \"-\"}";
  features += "]";
  int result = -999;
  # if SSL==true
    result = register_secure_server(wifi_client, mac_address, features);
  # else 
    result = register_insecure_server(wifi_client, mac_address, features);
  # endif
  Serial.print("setup - register returned result=");
  Serial.println(result);
  if (result != 0) {
    Serial.printf("setup - register error, returned result = %d\n", result);
    return;
  }

  // 5. read UUID from preferences
  //    if it's the first boot, it will be the same already stored in global variable 'saved_uuid',
  //    because already filled during the registration process.
  //    otherwise, it reads the existing UUID form preferences,
  //    because registration won't return the UUID again.
  Serial.println("setup - getting saved UUID from preferences...");
  saved_uuid = storage_get_uuid();
  if (saved_uuid.equals("")) {
    Serial.println("************* ERROR **************");
    Serial.println("setup - Cannot read saved UUID from Preferences");
    Serial.println("**********************************");
    return;
  }

  // 6. init IR library
  // Run the calibration to calculate uSec timing offsets for this platform.
  // This will produce a 65ms IR signal pulse at 38kHz.
  // Only ever needs to be run once per object instantiation, if at all.
  ir_init();
  
  delay(1000);
}

void loop() {
  // if 'saved_uuid' is not defined, it's an unregistered device
  if (saved_uuid == NULL || saved_uuid.length() == 0) {
    Serial.println("loop - saved_uuid NOT FOUND, cannot continue...");
    delay(60000);
    return;
  }

  // if not connected to the wifi, try to reconnect
  if (wifi_get_status() != WL_CONNECTED) {
    Serial.println("loop - WiFi connection lost!");
    wifi_reconnect();
  }

  // if not connected to mqtt server, try to reconnect
  if (!mqtt_client.connected()) {
    Serial.println("loop - mqtt connecting...");
    mqtt_connect(saved_uuid);
  }

  mqtt_client.loop();

  delay(1000);
}