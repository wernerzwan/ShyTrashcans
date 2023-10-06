// Packages for OSC
#include "WiFi.h"
#include <OSCMessage.h>

WiFiUDP Udp;  // A UDP instance to let us send and receive packets over UDP

// Network settings
const char* ssid = "SSID";            // Your network SSID (name)
const char* pass = "password";  // Your network password
const unsigned int localPort = 8888;  // Local port to listen for OSC packets

// UDP setup
const int update_rate = 16;
unsigned long UDPStartTime;

// Sensor PIN
const int PIRPin = 27;  // PIR sensor pin
bool motionDetected = false;

// LEDC setup
const int freq = 18000;
const int resolution = 8;

// Special sequence settings
bool specialSequenceTriggered = false;
const int specialTrashcan = 2;
const unsigned long specialDelayTime = 120000;

// Lamp setup
const int lampPin = 32;
unsigned long previousMillis = 0;
const int fadeInterval = 15;  // Interval between brightness changes
const int LChannel = 0;

// Defining the states for the switch
enum TrashcanState { CLOSING,
                     REVERSE,
                     CLOSED,
                     OPENING,
                     STAY_OPEN,
                     OPEN };

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
};

// Setting up the values of the variables
#define NUM_TRASHCANS 5


Trashcan trashcans[NUM_TRASHCANS] = {
  { CLOSING, false, 0, 15000, 2700, 13, 16, 250, 1, 2, 33 },
  { CLOSING, false, 0, 14000, 2000, 17, 18, 250, 3, 4, 34 },
  { CLOSING, false, 0, 12000, 2000, 19, 21, 250, 5, 6, 35 },
  { CLOSING, false, 0, 17000, 2000, 22, 23, 250, 7, 8, 39 },
  { CLOSING, false, 0, 13000, 2000, 25, 26, 250, 9, 10, 4 }
};

void setup() {

  // Setup WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  // Setup UDP
  Udp.begin(localPort);

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

  //check PIR Sensor
  motionDetected = digitalRead(PIRPin);

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
        if (millis() - trashcan.stateStartTime >= 50) {
          ledcWrite(trashcan.RChannel, 100);
        }
        if (millis() - trashcan.stateStartTime >= trashcan.reverseTime) {
          ledcWrite(trashcan.RChannel, 0);
          trashcan.state = CLOSED;
          trashcan.stateStartTime = millis();
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
        break;

      case OPENING:
        if ((motionDetected && !specialSequenceTriggered) || (specialSequenceTriggered && i != specialTrashcan)) {
          trashcan.state = CLOSING;
        } else {
          ledcWrite(trashcan.FChannel, 70);
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
        } else if (motionDetected || specialSequenceTriggered) {
          trashcan.state = CLOSING;
        }
        if (millis() - trashcan.stateStartTime >= specialDelayTime) {
          specialSequenceTriggered = false;
        }
        break;
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
      if (inmsg.isInt(0) && inmsg.getInt(0) == 1) {
        // Set the flag to trigger the special sequence
        specialSequenceTriggered = true;
      }
    }
  }
}
