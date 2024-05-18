#include <IRac.h>
#include <IRmacros.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRtext.h>
#include <IRtimer.h>
#include <IRutils.h>
#include <i18n.h>
#include <ir_Airton.h>
#include <ir_Airwell.h>
#include <ir_Amcor.h>
#include <ir_Argo.h>
#include <ir_Bosch.h>
#include <ir_Carrier.h>
#include <ir_Coolix.h>
#include <ir_Corona.h>
#include <ir_Daikin.h>
#include <ir_Delonghi.h>
#include <ir_Ecoclim.h>
#include <ir_Electra.h>
#include <ir_Fujitsu.h>
#include <ir_Goodweather.h>
#include <ir_Gree.h>
#include <ir_Haier.h>
#include <ir_Hitachi.h>
#include <ir_Kelon.h>
#include <ir_Kelvinator.h>
#include <ir_LG.h>
#include <ir_Magiquest.h>
#include <ir_Midea.h>
#include <ir_Mirage.h>
#include <ir_Mitsubishi.h>
#include <ir_MitsubishiHeavy.h>
#include <ir_NEC.h>
#include <ir_Neoclima.h>
#include <ir_Panasonic.h>
#include <ir_Rhoss.h>
#include <ir_Samsung.h>
#include <ir_Sanyo.h>
#include <ir_Sharp.h>
#include <ir_Tcl.h>
#include <ir_Technibel.h>
#include <ir_Teco.h>
#include <ir_Toshiba.h>
#include <ir_Transcold.h>
#include <ir_Trotec.h>
#include <ir_Truma.h>
#include <ir_Vestel.h>
#include <ir_Voltas.h>
#include <ir_Whirlpool.h>
#include <ir_York.h>

// include Arduino library to use Arduino function in cpp files
#include <Arduino.h>
// include json library (https://github.com/bblanchon/ArduinoJson)
#include <ArduinoJson.h>

// include libraries
// - IRremoteESP8266: https://github.com/crankyoldgit/IRremoteESP8266
#include <IRremoteESP8266.h>
#include <IRsend.h>
// Import the specific implementation to use LG protocol to control LG ACs
#include <ir_LG.h>

// private functions
void ir_send_signal();

// ------------------------------------------------------
// ------------------ IRremoteESP8266 -------------------
// GPIO pin to use to send IR signals
#define IR_SEND_PIN 4
// ------------------------------------------------------
// ------------------ LG protocol -----------------------
#define SEND_LG
// Temoerature ranges
#define TEMP_MIN 18 // forced to 18, because kLgAcMinTemp=16
#define TEMP_MAX kLgAcMaxTemp // 30
// Mode possibile values (defined in ir_LG.h)
#define MODE_COOL kLgAcCool // 0
#define MODE_DRY kLgAcDry // 1
#define MODE_FAN kLgAcFan // 2
#define MODE_AUTO kLgAcAuto // 3
#define MODE_HEAT kLgAcHeat // 4
// Fan values (defined in ir_LG.h)
#define FAN_MAX kLgAcFanHigh // 10
#define FAN_MED kLgAcFanMedium // 2
#define FAN_MIN kLgAcFanLowest // 0
#define FAN_AUTO kLgAcFanAuto // 5

// global initial state
struct state {
  bool powerStatus = false;
  uint8_t temperature = TEMP_MAX;
  uint8_t operation = MODE_COOL; // mode (heat, cold, ...)
  uint8_t fan = FAN_AUTO;
};
state acState;
 // Create a A/C object using GPIO to sending messages with
IRLgAc ac(IR_SEND_PIN);
// ------------------------------------------------------
// ------------------------------------------------------

void ir_init() {
  // set model version
  // based on your remote controller
  ac.setModel(lg_ac_remote_model_t::AKB74955603);

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
        Serial.println("ir_send_command - Auto0 not supported on tjis device");
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