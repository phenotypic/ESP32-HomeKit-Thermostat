#include <Arduino.h>
#include "HomeSpan.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

// Pin assignments
const int ONE_WIRE_BUS = 4;
const int RELAY_HEAT_PIN = 16;
const int RELAY_WATER_PIN = 17;

// Define relay states
#define RELAY_ON HIGH
#define RELAY_OFF LOW

// Thermostat configuration
const float TEMP_HYSTERESIS = 0.2;
const unsigned long TEMP_READ_INTERVAL = 5000;

// Preferences for storing settings
Preferences prefs;

// OneWire and temperature sensor setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Global variables
float currentTemp = 0.0;
bool heatingOn = false;
bool hotWaterOn = false;

// Helper function to set relay state and update global variable
void setRelay(int pin, bool state, bool &globalState, const char* name) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
  globalState = state;
  LOG1("%s %s\n", name, state ? "ON" : "OFF");
}

// ---------------------- HomeKit Services ----------------------

// HomeKit Thermostat Service
struct HeatingThermostat : Service::Thermostat {
  SpanCharacteristic *currentTempChar;
  SpanCharacteristic *targetTempChar;
  SpanCharacteristic *currentStateChar;
  SpanCharacteristic *targetStateChar;
  SpanCharacteristic *displayUnitsChar;
  unsigned long lastTempRead = -TEMP_READ_INTERVAL;

  HeatingThermostat() : Service::Thermostat() {
    float savedTargetTemp = prefs.getFloat("targetTemp", 20.5);
    int savedTargetState = prefs.getInt("targetState", 0);
    LOG1("Restored thermostat state: TargetTemp=%.1fC Mode=%d\n", savedTargetTemp, savedTargetState);

    currentTempChar = new Characteristic::CurrentTemperature(currentTemp);
    targetTempChar = new Characteristic::TargetTemperature(savedTargetTemp);
    targetTempChar->setRange(10, 30.0, 0.5);
    currentStateChar = new Characteristic::CurrentHeatingCoolingState(0);
    targetStateChar = new Characteristic::TargetHeatingCoolingState(savedTargetState);
    targetStateChar->setValidValues(2, 0, 1);
    displayUnitsChar = new Characteristic::TemperatureDisplayUnits();

    updateHeatingState(savedTargetTemp, savedTargetState);
  }

  boolean update() override {
    float newTargetTemp = targetTempChar->getNewVal();
    int newTargetState = targetStateChar->getNewVal();

    prefs.putFloat("targetTemp", newTargetTemp);
    prefs.putInt("targetState", newTargetState);

    LOG1("HomeKit update: TargetTemp=%.1fC Mode=%d\n", newTargetTemp, newTargetState);
    updateHeatingState(newTargetTemp, newTargetState);
    return true;
  }

  void loop() override {
    if (millis() - lastTempRead >= TEMP_READ_INTERVAL) {
      lastTempRead = millis();
      sensors.requestTemperatures();
      float tempC = sensors.getTempCByIndex(0);

      if (tempC != DEVICE_DISCONNECTED_C) {
        currentTemp = tempC;
        currentTempChar->setVal(currentTemp);
        LOG1("Ambient temperature: %.2f C\n", currentTemp);
      } else {
        LOG1("Temperature sensor disconnected\n");
      }
    }
    updateHeatingState(targetTempChar->getVal<float>(), targetStateChar->getVal());
  }

  void updateHeatingState(float targetTemp, int targetMode) {
    if (targetMode == 0) {
      if (heatingOn) {
        LOG1("Thermostat mode OFF -> turning heating OFF\n");
        setRelay(RELAY_HEAT_PIN, false, heatingOn, "Heating");
        currentStateChar->setVal(0);
      }
      return;
    }

    float diff = targetTemp - currentTemp;

    if (diff > TEMP_HYSTERESIS && !heatingOn) {
      LOG1("Heating ON (current=%.2fC target=%.2fC diff=%.2fC)\n", currentTemp, targetTemp, diff);
      setRelay(RELAY_HEAT_PIN, true, heatingOn, "Heating");
      currentStateChar->setVal(1);
    } else if (diff < -TEMP_HYSTERESIS && heatingOn) {
      LOG1("Heating OFF (current=%.2fC target=%.2fC diff=%.2fC)\n", currentTemp, targetTemp, diff);
      setRelay(RELAY_HEAT_PIN, false, heatingOn, "Heating");
      currentStateChar->setVal(0);
    }
  }
};

// HomeKit Switch Service (Hot Water)
struct HotWaterSwitch : Service::Switch {
  SpanCharacteristic *power;

  HotWaterSwitch() : Service::Switch() {
    bool savedState = prefs.getBool("hotWaterOn", false);
    LOG1("Restored hot water state: %s\n", savedState ? "ON" : "OFF");
    
    power = new Characteristic::On(savedState);
    setRelay(RELAY_WATER_PIN, savedState, hotWaterOn, "Hot Water");
  }

  boolean update() override {
    bool newState = power->getNewVal();
    LOG1("HomeKit update: Hot Water=%s\n", newState ? "ON" : "OFF");
    prefs.putBool("hotWaterOn", newState);
    setRelay(RELAY_WATER_PIN, newState, hotWaterOn, "Hot Water");
    return true;
  }
};

// ---------------------- Setup & Loop ----------------------

void setup() {
  Serial.begin(115200);
  prefs.begin("thermostat", false);
  sensors.begin();

  pinMode(RELAY_HEAT_PIN, OUTPUT);
  pinMode(RELAY_WATER_PIN, OUTPUT);

  homeSpan.setPairingCode("11122333");
  homeSpan.setPortNum(1201);
  homeSpan.setStatusPin(23);
  homeSpan.setLogLevel(1);
  homeSpan.begin(Category::Thermostats, "ESP32 Thermostat");

  // Define accessory
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Thermostat");
      new Characteristic::Manufacturer("DIY");
      new Characteristic::SerialNumber("001234");
      new Characteristic::Model("ESP32-HeatHotWater");
      new Characteristic::FirmwareRevision("1.0");
      new Characteristic::Identify();
    
    new HotWaterSwitch();
    new HeatingThermostat();

  LOG1("Setup complete. Waiting for HomeKit pairing...\n");
}

void loop() {
  homeSpan.poll();
}
