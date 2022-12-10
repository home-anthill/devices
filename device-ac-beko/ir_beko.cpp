// include Arduino library to use Arduino function in cpp files
#include <Arduino.h>
// include json library (https://github.com/bblanchon/ArduinoJson)
#include <ArduinoJson.h>

// include libraries
// - IRremoteESP8266: https://github.com/crankyoldgit/IRremoteESP8266
#include <IRremoteESP8266.h>
#include <IRsend.h>
// Import the specific implementation to use COOLIX protocol to control Beko ACs
#include <ir_Coolix.h>

// private functions
void ir_send_signal();

// ------------------------------------------------------
// ------------------ IRremoteESP8266 -------------------
// GPIO pin to use to send IR signals
#define IR_SEND_PIN 4
// ------------------------------------------------------
// ---------------- COOLIX protocol ---------------------
#define SEND_COOLIX
// Temoerature ranges
#define TEMP_MIN kCoolixTempMin // 17
#define TEMP_MAX kCoolixTempMax // 30
// Mode possibile values (defined in ir_Coolix.h)
#define MODE_COOL kCoolixCool // 0
#define MODE_DRY kCoolixDry // 1
#define MODE_AUTO kCoolixAuto // 2
#define MODE_HEAT kCoolixHeat // 3
#define MODE_FAN kCoolixFan // 4
// Fan values (defined in ir_Coolix.h)
#define FAN_AUTO0 kCoolixFanAuto0 // 0
#define FAN_MAX kCoolixFanMax // 1
#define FAN_MED kCoolixFanMed // 2
#define FAN_MIN kCoolixFanMin // 4
#define FAN_AUTO kCoolixFanAuto // 5
// global initial state
struct state {
  bool powerStatus = false;
  uint8_t temperature = TEMP_MAX;
  uint8_t operation = MODE_COOL; // mode (heat, cold, ...)
  uint8_t fan = FAN_AUTO0;
};
state acState;
 // Create a A/C object using GPIO to sending messages with
IRCoolixAC ac(IR_SEND_PIN);
// ------------------------------------------------------
// ------------------------------------------------------

void ir_init() {
  ac.calibrate();
  delay(1000);
  // Start AC
  ac.begin();
}

void ir_send_command(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<250> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("ir_send_command - deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }
  const char* uuid = doc["uuid"];
  const char* profileToken = doc["profileToken"];
  Serial.print("ir_send_command - uuid: ");
  Serial.println(uuid);
  Serial.print("ir_send_command - profileToken: ");
  Serial.println(profileToken);

  // ArduinoJson developer says: "don't use doc.containsKey("on"), instead use doc["on"] != nullptr"
  if(doc["on"] != nullptr) {
    const bool on = doc["on"];
    Serial.print("ir_send_command - on: ");
    Serial.println(on);
    if (on == 1) {
      Serial.println("ir_send_command - setting On");
      ac.on();
    } else if (on == 0) {
      Serial.println("ir_send_command - setting Off");
      ac.off();
      // because OFF is a special fixed command, and you cannot set any other parameters
      ir_send_signal();
      Serial.println("--------------------------");
      return;
    }
  }
  if(doc["temperature"] != nullptr) {
    const int temperature = doc["temperature"];
    Serial.print("ir_send_command - temperature: ");
    Serial.println(temperature);

    if (temperature < TEMP_MIN || temperature > TEMP_MAX) {
      Serial.print("ir_send_command - cannot set value, because temperature is out of range. Temperature must be >= ");
      Serial.print(TEMP_MIN);
      Serial.print(" and <= ");
      Serial.print(TEMP_MAX);
      Serial.print("\n");
      return;
    }
    Serial.println("ir_send_command - setting temperature");
    ac.setTemp(temperature);
  }
  if(doc["mode"] != nullptr) {
    const int mode = doc["mode"];
    Serial.print("ir_send_command - mode: ");
    Serial.println(mode);
    switch(mode) {
      case 1:
       Serial.println("ir_send_command - setting mode to Cool");
        ac.setMode(MODE_COOL);
        break;
      case 2:
        Serial.println("ir_send_command - setting mode to Auto");
        ac.setMode(MODE_AUTO);
        break;
      case 3:
        Serial.println("ir_send_command - setting mode to Heat");
        ac.setMode(MODE_HEAT);
        break;
      case 4:
        Serial.println("ir_send_command - setting mode to Fan");
        ac.setMode(MODE_FAN);
        break;
      case 5:
        Serial.println("ir_send_command - setting mode to Dry");
        ac.setMode(MODE_DRY);
        break;
      default:
        Serial.println("ir_send_command - cannot set mode. Unsupported value!");
        break;
    }
  }
  if(doc["fanSpeed"] != nullptr) {
    const int fanSpeed = doc["fanSpeed"];
    Serial.print("ir_send_command - fanSpeed: ");
    Serial.println(fanSpeed);
    switch(fanSpeed) {
      case 1:
        Serial.println("ir_send_command - setting fan speed to Min");
        ac.setFan(FAN_MIN);
        break;
      case 2:
        Serial.println("ir_send_command - setting fan speed to Med");
        ac.setFan(FAN_MED);
        break;
      case 3:
        Serial.println("ir_send_command - setting fan speed to Max");
        ac.setFan(FAN_MAX);
        break;
      case 4:
        Serial.println("ir_send_command - setting fan speed to Auto");
        ac.setFan(FAN_AUTO);
        break;
      case 5:
        Serial.println("ir_send_command - setting fan speed to Auto0");
        ac.setFan(FAN_AUTO0);
        break;
      default:
        Serial.println("ir_send_command - cannot set fan speed. Unsupported fan value!");
        break;
    }
  }
  ir_send_signal();
  Serial.println("--------------------------");
}

void ir_send_signal() {
  Serial.println("ir_send_signal - sending value via IR...");
  ac.send();
  Serial.println("ir_send_signal - value sent successfully!");
}