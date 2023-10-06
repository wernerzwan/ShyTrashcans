// Packages for OSC
#include "WiFi.h"
#include <OSCMessage.h>

WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

// Network settings
char ssid[] = "SSID"; // your network SSID (name)
char pass[] = "password";  // your network password
unsigned int localPort = 8888; // local port to listen for OSC packets

// Setting up UDP 
int update_rate = 16;
unsigned long UDPStartTime;

// Sensor PIN
const int PIRPin = 27; // PIR sensor pin
bool motionDetected = false;

// Setting Up ESP32 ledc function
const int freq = 18000;
const int resolution = 8;

// specialSequence Settings
bool specialSequenceTriggered = true;
int specialTrashcan = 2;
unsigned long specialDelayTime = 120000;

// Trashcan 1 
const int TrashcanPin1 = 13;
const int FChannel1 = 1;
const int ReversePin1 = 16;
const int RChannel1 = 2;
const int HALPin1 = 33;

// Trashcan 2
const int TrashcanPin2 = 17;
const int FChannel2 = 3;
const int ReversePin2 = 18;
const int RChannel2 = 4;
const int HALPin2 = 34;

// Trashcan 3
const int TrashcanPin3 = 19;
const int FChannel3 = 5;
const int ReversePin3 = 21;
const int RChannel3 = 6;
const int HALPin3 = 35;

// Trashcan 4
const int TrashcanPin4 = 22;
const int FChannel4 = 7;
const int ReversePin4 = 23;
const int RChannel4 = 8; 
const int HALPin4 = 39;

// Trashcan 5
const int TrashcanPin5 = 25;
const int FChannel5 = 9;
const int ReversePin5 = 26;
const int RChannel5 = 10; 
const int HALPin5 = 4;

// Defining the states for the switch
enum TrashcanState { CLOSING, REVERSE, CLOSED, OPENING, STAY_OPEN, OPEN };

// Variables per trashcan
struct Trashcan {
  TrashcanState state;
  bool bottomReached;
  unsigned long stateStartTime;
  unsigned long delayTime;
  unsigned long openingTime;
  int trashcanPin;
  int reversePin;
  int FChannel;
  int RChannel;
  int hallPin;
};

// Setting up the values of the variables
#define NUM_TRASHCANS 5

Trashcan trashcans[NUM_TRASHCANS] = {
  {CLOSING, false, 0, 15000, 2700, TrashcanPin1, ReversePin1, FChannel1, RChannel1, HALPin1},
  {CLOSING, false, 0, 14000, 2000, TrashcanPin2, ReversePin2, FChannel2, RChannel2, HALPin2},
  {CLOSING, false, 0, 12000, 2000, TrashcanPin3, ReversePin3, FChannel3, RChannel3, HALPin3},
  {CLOSING, false, 0, 17000, 2000, TrashcanPin4, ReversePin4, FChannel4, RChannel4, HALPin4},
  {CLOSING, false, 0, 13000, 2000, TrashcanPin5, ReversePin5, FChannel5, RChannel5, HALPin5}
};

void setup() {

  /* setup wifi */
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
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

  pinMode(PIRPin, INPUT);
}

void loop() {

  //check UDP messages at localPort 
  if(millis() - UDPStartTime > update_rate){
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
          //digitalWrite(LED, LOW);
          ledcWrite(trashcan.FChannel, 255);
        }
        break;

      case REVERSE:
        ledcWrite(trashcan.FChannel, 0);
        if (millis() - trashcan.stateStartTime >= 50) {
          ledcWrite(trashcan.RChannel, 100);
        }
        if (millis() - trashcan.stateStartTime >= 250) {
          ledcWrite(trashcan.RChannel, 0);
          trashcan.state = CLOSED;
          trashcan.stateStartTime = millis();
          Serial.println("Reversing");
        }
        break;

      case CLOSED:
        ledcWrite(trashcan.FChannel, 0);
        if (specialSequenceTriggered && i == specialTrashcan){
          trashcan.state = OPENING;
          trashcan.stateStartTime = millis();
        } else if (specialSequenceTriggered && i != specialTrashcan){
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
          if (millis() - trashcan.stateStartTime >= specialDelayTime) {
          specialSequenceTriggered = false;
          } } else if (motionDetected || specialSequenceTriggered) {
          trashcan.state = CLOSING;
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
