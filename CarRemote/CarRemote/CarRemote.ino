#include <CapacitiveSensor.h>

#define null 0

CapacitiveSensor   cs_5_2 = CapacitiveSensor(5,2);
const int analogInPin = A3;  // The CAR switch
const int powerPin = 10;
const unsigned long firstDebounceDelay = 1000;
const unsigned long debounceDelay = 200;
const unsigned long stableDelay = 100;

struct TSwitchMap {
  int min;
  int max;
  const char* data;
};

const TSwitchMap switchMap[] = {
  { 600, 700, "up" },
  { 700, 800, "dn" },
  { 800, 900, "vol+" },
  { 900, 1000, "vol-" },
  { 1000, 1024, "test" }
};

void processSteeringRemote();
void processCapsRemote();
void powerButtonDown();
void powerButtonUp();

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  cs_5_2.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_5_2.set_CS_Timeout_Millis(500);
  pinMode(powerPin, INPUT);
}

String serialBuffer;
void loop() {
  // We attach GPS TX to arduino RX, and arduino TX to OrangePI RX
  // Redirect it
  if (Serial.available()) {
    const char c = Serial.read();
    serialBuffer += c;
    if (c == '\n') {
      Serial.print(serialBuffer);
      serialBuffer = "";
    }
  } else {
    processSteeringRemote();
    processCapsRemote();
  }
}

const char* lastButton = null;
unsigned long lastDebounceTime = 0;
unsigned long nextDelay = 0;
void processSteeringRemote() {
  int sensorValue = analogRead(analogInPin);
  const char* curButton = null;
  for (int i = 0; i < sizeof(switchMap) / sizeof(TSwitchMap); i++) {
    if (sensorValue > switchMap[i].min && sensorValue < switchMap[i].max) {
      curButton = switchMap[i].data;
      break;
    }
  }

  if (curButton == null) {
    lastButton = null;
    lastDebounceTime = millis();
    nextDelay = stableDelay;
    return;
  }

  if (millis() - lastDebounceTime > nextDelay) {
    Serial.print("$BTNPR,");
    Serial.print(curButton);
    Serial.write("\n", 1);
    
    lastDebounceTime = millis();
    if (lastButton != curButton) {
      lastButton = curButton;
      nextDelay = firstDebounceDelay;
    } else {
      nextDelay = debounceDelay;
    }
  }
}

unsigned long capsLastDebounceTime = 0;
unsigned long capsPowerResetTime = 0;
unsigned long capsNextDelay = 0;
unsigned long capsLastReset = 0;
const char* capsLastButton = null;
int capsBtnPower = 0;
bool capsBtnBack = false;
void processCapsRemote() {
  const uint8_t samples = 50;
  const char* BTN_POWER = "power";
  const char* BTN_BACK = "back";

  if (millis() - capsLastReset > 60000) {
    cs_5_2.reset_CS_AutoCal();
    capsLastReset = millis();
  }
  
  long sensorValue =  cs_5_2.capacitiveSensor(samples);
  //Serial.println(sensorValue);
  
  if (sensorValue > 100 && sensorValue < 170) {
    capsBtnPower++;
    capsNextDelay = 500;
    capsLastDebounceTime = millis();
  } else if (sensorValue > 4000) {
    capsBtnPower = 0;
    capsBtnBack = true;
    capsNextDelay = 100;
    capsLastDebounceTime = millis();
  } else {
    capsBtnPower = 0;
  }

  if (millis() - capsLastDebounceTime > capsNextDelay) {
    capsBtnPower = 0;
    capsBtnBack = false;
    capsLastButton = null;
    capsPowerResetTime = 0;
    powerButtonUp();
  }

  if ((capsPowerResetTime > 0) && (millis() - capsPowerResetTime > 500)) {
    powerButtonUp();
    capsPowerResetTime = 0;
  }

  if (capsBtnPower > 50 && capsLastButton != BTN_POWER) {
    capsLastButton = BTN_POWER;
    Serial.print("$BTNPR,");
    Serial.println(BTN_POWER);
    powerButtonDown();
    capsPowerResetTime = millis();
  } else if (capsBtnBack && capsLastButton != BTN_BACK) {
    capsLastButton = BTN_BACK;
    Serial.print("$BTNPR,");
    Serial.println(BTN_BACK);
  }
}

void powerButtonDown() {
    // Switch to output low
    digitalWrite(10, LOW);
    pinMode(10, OUTPUT);
    digitalWrite(10, LOW);
}

void powerButtonUp() {
    pinMode(10, INPUT);
    digitalWrite(10, LOW);
}
