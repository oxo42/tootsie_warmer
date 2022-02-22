// John Oxley
// Over-engineered boot warmer

#include <Arduino.h>
#include <AsyncTimer.h>
#include <EspMQTTClient.h>
#include <LiquidCrystal.h>
#include <ESP8266Wifi.h>
#include <HAMqttDevice.h>

#include "tootsie.h"

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


HAMqttDevice tootsie_timer("tootsie_timer", HAMqttDevice::SENSOR, "homeassistant");

void logTimeLeft();
void onSetMessageReceived(const String &payload);
void stop();


void sendConfig()
{
  client.publish(tootsie_timer.getConfigTopic(), tootsie_timer.getConfigPayload());
  logTimeLeft();
}

void sendDurationRemaining(unsigned int seconds_left)
{
  tootsie_timer.clearAttributes();
  tootsie_timer.addAttribute("duration", "the duration");
  tootsie_timer.addAttribute("remaining", String(seconds_left));
  client.publish(tootsie_timer.getAttributesTopic(), tootsie_timer.getAttributesPayload());
  if (seconds_left > 0)
    client.publish(tootsie_timer.getStateTopic(), "active");
  else
    client.publish(tootsie_timer.getStateTopic(), "idle");
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

  HAMqttDeviceRegistry dev;
  dev.addAttribute("name", "tootsie");
  dev.addAttribute("model", "TootsieWarmer");
  dev.addAttribute("manufacturer", "Ox");
  dev.addIdentifier(WiFi.macAddress());

  tootsie_timer.addConfigVar("expire_after", "180");
  tootsie_timer.addConfigVar("dev", dev.getPayload());
  tootsie_timer.enableAttributesTopic();

  Serial.println("Initialisation Complete");
}

void logTimeLeft()
{
  if (timer_id > 0)
  {
    unsigned long seconds_left = t.getRemaining(timer_id) / 1000;
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