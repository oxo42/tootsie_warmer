// John Oxley
// Over-engineered boot warmer

#include <Arduino.h>
#include <AsyncTimer.h>
#include <EspMQTTClient.h>
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <ESP8266Wifi.h>

#include "tootsie.h"

#define DEBUG 0

#define RELAY_PIN D4
#define BUTTON_ADD D3
#define BUTTON_STOP D2
#define ADD_SECONDS 15 * 60
#define MAX_SECONDS 180 * 60

#define EXPIRE_AFTER "180"

EspMQTTClient client(
    "LAN Solo",
    "smallandpoopy",
    "10.0.0.2",
    "BootWarmer" // Client name that uniquely identify your device
);
AsyncTimer t;
int timer_id = 0;
int log_timer_id = 0;
int total_time_ms = 0;
LiquidCrystal lcd(D1, D8, D0, D7, D6, D5);

const String SENSOR_ID{"tootsie"};

const String state_topic{"homeassistant/sensor/" + SENSOR_ID + "/state"};
const String attr_topic{"homeassistant/sensor/" + SENSOR_ID + "/attributes"};

void logTimeLeft();
void onSetMessageReceived(const String &payload);
void stop();

void publishEntity(String entityType, String entity, String unit, String deviceClass = "None")
{
    DynamicJsonDocument configPayload(1024);
    configPayload["stat_t"] = state_topic.c_str();
    // configPayload["expire_after"] = EXPIRE_AFTER;
    configPayload["dev"]["name"] = SENSOR_ID.c_str();
    configPayload["dev"]["model"] = "TootsieWarmer";
    configPayload["dev"]["manufacturer"] = "Ox";
    JsonArray identifiers{configPayload["dev"].createNestedArray("ids")};
    identifiers.add(WiFi.macAddress());

    const String unique_id{SENSOR_ID + "_" + entity};
    const String value_template{"{{value_json['" + entity + "']}}"};

    configPayload["name"] = unique_id.c_str();
    configPayload["uniq_id"] = unique_id.c_str();
    configPayload["val_tpl"] = value_template.c_str();
    configPayload["unit_of_meas"] = unit.c_str();
    if (deviceClass != "None")
        configPayload["device_class"] = deviceClass.c_str();

    const String config_topic_entity{"homeassistant/" + entityType + "/" + SENSOR_ID + "_" + entity + "/config"};

    char configPayloadSerialized[1024]{};
    serializeJson(configPayload, configPayloadSerialized);
    if (DEBUG)
    {
        Serial.println(config_topic_entity.c_str());
        Serial.println(configPayloadSerialized);
    }

    // Publishing
    if (!client.publish(config_topic_entity.c_str(), configPayloadSerialized, true))
        Serial.println("Config entity NOT published");
}

void sendConfig()
{
  publishEntity("sensor", "duration", "s");
  logTimeLeft();
}

void sendDurationRemaining(unsigned int seconds_left)
{
  DynamicJsonDocument sensorData(1024);
  sensorData["duration"] = seconds_left;

  char buffer[1024]{""};
  serializeJson(sensorData, buffer);
  if (DEBUG)
    Serial.println(buffer);

  if (!client.publish(state_topic.c_str(), buffer, true))
    Serial.println("state NOT published");
}

void onConnectionEstablished()
{
  Serial.println("MQTT Connection Established");
  sendConfig();
  client.subscribe("homeassistant/sensor/tootsie/set", onSetMessageReceived);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println(x());
  Serial.println("Setting up buttons");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_ADD, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Tootsie Warmer!");
  lcd.setCursor(0, 1);
  lcd.print("Idle");

  client.setMaxPacketSize(1024);
  Serial.println("Initialisation Complete");
}

void logTimeLeft()
{
  if (timer_id > 0)
  {
    unsigned long seconds_left = t.getMsLeft(timer_id) / 1000;
    sendDurationRemaining(seconds_left);
    Serial.print("Seconds left: ");
    Serial.println(seconds_left);
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(seconds_left);
    lcd.print("s            ");
  }
  else
  {
    sendDurationRemaining(0);
  }
}

void addTime(int add_ms)
{
  Serial.print("Adding ");
  Serial.print(add_ms);
  Serial.println(" ms.");

  digitalWrite(RELAY_PIN, HIGH);
  if (timer_id > 0)
  {
    if ((total_time_ms + add_ms) <= MAX_SECONDS * 1000)
    {
      // Timer exists, add time
      t.delay(timer_id, add_ms);
      total_time_ms += add_ms;
    }
    else
    {
      Serial.println("Time too large, refusing to add");
    }
  }
  else
  {
    timer_id = t.setTimeout(stop, add_ms);
    log_timer_id = t.setInterval(logTimeLeft, 1000);
    total_time_ms = add_ms;
  }
  logTimeLeft();
}

void setTime(int ms)
{
  Serial.print("Setting timer to: ");
  Serial.println(ms);
  stop();
  addTime(ms);
}

void stop()
{
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Stopping timer");
  sendDurationRemaining(0);
  if (timer_id > 0)
  {
    t.cancel(timer_id);
  }
  t.cancel(log_timer_id);
  timer_id = 0;
  lcd.setCursor(0, 1);
  lcd.print("Idle           ");
}

void loop()
{
  t.handle();
  client.loop();

  if (digitalRead(BUTTON_ADD) == LOW)
  {
    addTime(ADD_SECONDS * 1000);
    delay(250);
  }
  if (digitalRead(BUTTON_STOP) == LOW)
  {
    stop();
    delay(250);
  }
}

void onSetMessageReceived(const String &payload)
{
  int ms = payload.toInt() * 1000;
  setTime(ms);
}