// Packages for OSC
#include "WiFi.h"
#include <OSCMessage.h>

WiFiUDP Udp;  // A UDP instance to let us send and receive packets over UDP

// Network settings
const char* ssid = "KE25";           // Your network SSID (name)
const char* pass = "MeltingIce2011"; // Your network password
const unsigned int localPort = 8888; // Local port to listen for OSC packets

// UDP setup
const int update_rate = 16;
unsigned long UDPStartTime;

// Sensor PIN
const int PIRPin = 13;  // PIR sensor pin
bool motionDetected = false;
bool previousMotionState = false;

// LEDC setup
const int freq = 18000;
const int resolution = 8;

// Special sequence settings
bool specialSequenceTriggered = false;
const int specialTrashcan = 3;
const unsigned long specialDelayTime = 120000;

// Lamp setup
const int lampPin = 5;
unsigned long previousMillis = 0;
const int fadeInterval = 50;  // Interval between brightness changes
const int LChannel = 0;

// Defining the states for the switch
enum TrashcanState { CLOSING,
                     REVERSE,
                     CLOSED,
                     OPENING,
                     STAY_OPEN,
                     OPEN,
                     ANGRY,
                     ANGRYCLOSING,
                     ANGRYREVERSE
                      };

// Variables per trashcan
struct Trashcan {
  TrashcanState state;
  bool bottomReached;
  unsigned long stateStartTime;
  unsigned long delayTime;
  unsigned long openingTime;
  int trashcanPin;
  int reversePin;
  int reverseTime;
  int FChannel;
  int RChannel;
  int hallPin;
  int slowSpeed;
  unsigned long angryTime;
  unsigned long angryStartTime;
};

// Setting up the values of the variables
#define NUM_TRASHCANS 5


Trashcan trashcans[NUM_TRASHCANS] = {
  { CLOSING, false, 0, 15000, 3100, 16, 17, 100, 1, 2, 4, 60, 80000, 0 },
  { CLOSING, false, 0, 14000, 400, 21, 19, 100, 3, 4, 18, 150, 51000, 0 },
  { CLOSING, false, 0, 12000, 500, 23, 22, 100, 5, 6, 12, 100, 34000, 0 },
  { CLOSING, false, 0, 17000, 1200, 32, 33, 100, 7, 8, 14, 60, 49000, 0 },
  { CLOSING, false, 0, 13000, 2400, 26, 27, 100, 9, 10, 25, 60, 91000, 0 }
};

void setup() {
Serial.begin(115200);
  // Setup WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(WiFi.localIP());
  // Setup UDP
  Udp.begin(localPort);
  ledcWrite(LChannel, 0);
  // Setup Trashcans
  for (int i = 0; i < NUM_TRASHCANS; i++) {
    pinMode(trashcans[i].trashcanPin, OUTPUT);
    pinMode(trashcans[i].reversePin, OUTPUT);
    pinMode(trashcans[i].hallPin, INPUT);
    ledcSetup(trashcans[i].FChannel, freq, resolution);
    ledcSetup(trashcans[i].RChannel, freq, resolution);
    ledcAttachPin(trashcans[i].trashcanPin, trashcans[i].FChannel);
    ledcAttachPin(trashcans[i].reversePin, trashcans[i].RChannel);
  }
  ledcSetup(LChannel, freq, resolution);
  ledcAttachPin(lampPin, LChannel);
  pinMode(PIRPin, INPUT);
}

void loop() {
  //check UDP messages
  if (millis() - UDPStartTime > update_rate) {
    receiveMessage();
    UDPStartTime = millis();
  }

  // Read the current state of the PIR sensor
  bool currentMotionState = digitalRead(PIRPin);

  // Check if the state has changed since the last reading
  if (currentMotionState != previousMotionState) {
    // Motion state has changed, update the previous state
    previousMotionState = currentMotionState;

    // Process the new state
    if (currentMotionState == HIGH) {
      // Motion detected
      motionDetected = true;
    } else {
      // No motion detected
      motionDetected = false;
    }
  }

  for (int i = 0; i < NUM_TRASHCANS; i++) {
    // picking trashcan i out of max 5
    Trashcan& trashcan = trashcans[i];
    // start the scenes
    switch (trashcan.state) {

      case CLOSING:
      
        trashcan.bottomReached = digitalRead(trashcan.hallPin);
        if (trashcan.bottomReached) {
          trashcan.state = REVERSE;
          trashcan.stateStartTime = millis();
        } else {
          ledcWrite(trashcan.FChannel, 255);
        }
        break;

      case REVERSE:
        ledcWrite(trashcan.FChannel, 0);
        if (millis() - trashcan.stateStartTime >= 100) {
          ledcWrite(trashcan.RChannel, 100);
        }
        if (millis() - trashcan.stateStartTime >= trashcan.reverseTime) {
          ledcWrite(trashcan.RChannel, 0);
          trashcan.state = CLOSED;
          trashcan.stateStartTime = millis();
          trashcan.angryStartTime = millis();
        }
        break;

      case CLOSED:
      
        ledcWrite(trashcan.FChannel, 0);
        if (specialSequenceTriggered && i == specialTrashcan) {
          trashcan.state = OPENING;
          trashcan.stateStartTime = millis();
        } else if (specialSequenceTriggered && i != specialTrashcan) {
          trashcan.stateStartTime = millis();
        } else if (motionDetected) {
          trashcan.stateStartTime = millis();
        }
        if (millis() - trashcan.stateStartTime >= trashcan.delayTime) {
          trashcan.state = OPENING;
          trashcan.stateStartTime = millis();
        }
        if (!specialSequenceTriggered && millis() - trashcan.angryStartTime >= trashcan.angryTime) {
          trashcan.state = ANGRY;
          trashcan.angryStartTime = millis();
        }
        break;

      case OPENING:
        if ((motionDetected && !specialSequenceTriggered) || (specialSequenceTriggered && i != specialTrashcan)) {
          trashcan.state = CLOSING;
        } else {
          ledcWrite(trashcan.FChannel, trashcan.slowSpeed);
          trashcan.bottomReached = false;
        }
        if (millis() - trashcan.stateStartTime >= trashcan.openingTime) {
          ledcWrite(trashcan.FChannel, 0);
          trashcan.state = OPEN;
          trashcan.stateStartTime = millis();
        }
        break;

      case OPEN:
        trashcan.bottomReached = false;
        if (specialSequenceTriggered && i == specialTrashcan) {
          for (int dutyCycle = 150; dutyCycle <= 255; dutyCycle++) {
            // Calculate the nextMillis when the LED brightness should be updated
            unsigned long nextMillis = previousMillis + fadeInterval;

            // Check if it's time to update the LED brightness
            if (millis() >= nextMillis) {
              previousMillis = millis();  // Save the current time

              // Changing the LED brightness with PWM
              ledcWrite(LChannel, dutyCycle);
            }
          }

          for (int dutyCycle = 255; dutyCycle >= 150; dutyCycle--) {
            // Calculate the nextMillis when the LED brightness should be updated
            unsigned long nextMillis = previousMillis + fadeInterval;

            // Check if it's time to update the LED brightness
            if (millis() >= nextMillis) {
              previousMillis = millis();  // Save the current time

              // Changing the LED brightness with PWM
              ledcWrite(LChannel, dutyCycle);
            }
          }
        } else if (motionDetected || specialSequenceTriggered && i != specialTrashcan) {
          trashcan.state = CLOSING;
        }
        Serial.print(millis());
        Serial.print(" ");
        Serial.println(trashcan.stateStartTime);
        if (millis() - trashcan.stateStartTime >= specialDelayTime) {
          specialSequenceTriggered = false;
          ledcWrite(LChannel, 0);
        break;

      case ANGRY:
      if(specialSequenceTriggered){
        trashcan.state = CLOSING;
        trashcan.stateStartTime = millis();
        trashcan.angryStartTime = millis();
      } else if(millis() - trashcan.angryStartTime >= random(1500, 4500)){
        trashcan.state = ANGRYCLOSING;
      } else {
        ledcWrite(trashcan.FChannel,255);
      }
      break;
      
      case ANGRYCLOSING:
      trashcan.bottomReached = digitalRead(trashcan.hallPin);
      if (trashcan.bottomReached) {
      trashcan.state = CLOSED;
      trashcan.angryTime = random(30000, 80000);
      trashcan.angryStartTime = millis();
        } else {
          ledcWrite(trashcan.FChannel, 255);
        }
        break;

      case ANGRYREVERSE:
        ledcWrite(trashcan.FChannel, 0);
        if (millis() - trashcan.angryStartTime >= 100) {
          ledcWrite(trashcan.RChannel, 100);
        }
        if (millis() - trashcan.angryStartTime >= trashcan.reverseTime) {
          ledcWrite(trashcan.RChannel, 0);
          trashcan.state = CLOSED;
          trashcan.angryStartTime = millis();
        }
        break;


    }
  }
}
}


void receiveMessage() {
  OSCMessage inmsg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      inmsg.fill(Udp.read());
    }
    if (!inmsg.hasError()) {
      // Check if the received message is "1"
    Serial.println("Got message!");
    specialSequenceTriggered = true;
      }
    }
  }
