const int TrashcanPin1 = 3;
const int ReversePin1 = 6;
const int TrashcanPin2 = 9;
const int ReversePin2 = 10;
const int LED = 13; // onboard LED pin
const int PIRPin = 5; // PIR sensor pin
const int HALPin1 = 2;
const int HALPin2 = 4;

enum TrashcanState { CLOSING, REVERSE, CLOSED, OPENING, OPEN };

struct Trashcan {
  TrashcanState state;
  bool bottomReached;
  unsigned long stateStartTime;
  unsigned long delayTime;
  unsigned long openingTime;
  int trashcanPin;
  int reversePin;
};

Trashcan trashcans[2] = {
  {CLOSING, false, 0, 3000, 2700, TrashcanPin1, ReversePin1},
  {CLOSING, false, 0, 8000, 2000, TrashcanPin2, ReversePin2}
};

bool motionDetected = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  for (int i = 0; i < 2; i++) {
    pinMode(trashcans[i].trashcanPin, OUTPUT);
    pinMode(trashcans[i].reversePin, OUTPUT);
  }
  pinMode(PIRPin, INPUT);
  pinMode(HALPin1, INPUT);
  pinMode(HALPin2, INPUT);
}

void loop() {
  motionDetected = digitalRead(PIRPin);

  for (int i = 0; i < 2; i++) {
    Trashcan& trashcan = trashcans[i];

    switch (trashcan.state) {
      case CLOSING:
        trashcan.bottomReached = digitalRead(i == 0 ? HALPin1 : HALPin2);
        if (trashcan.bottomReached) {
          trashcan.state = REVERSE;
          trashcan.stateStartTime = millis();
        } else {
          digitalWrite(LED, LOW);
          analogWrite(trashcan.trashcanPin, 255);
        }
        break;

      case REVERSE:
        analogWrite(trashcan.trashcanPin, 0);
        if (millis() - trashcan.stateStartTime >= 50) {
          analogWrite(trashcan.reversePin, 100);
        }
        if (millis() - trashcan.stateStartTime >= 250) {
          analogWrite(trashcan.reversePin, 0);
          trashcan.state = CLOSED;
          trashcan.stateStartTime = millis();
          Serial.println("Reversing");
        }
        break;

      case CLOSED:
        analogWrite(trashcan.trashcanPin, 0);
        if (motionDetected) {
          trashcan.stateStartTime = millis();
          Serial.println("Motion Detected! Resetting Timer.");
        }
        Serial.print("Time remaining until next attempt:");
        Serial.println(trashcan.delayTime + trashcan.stateStartTime - millis());
        if (millis() - trashcan.stateStartTime >= trashcan.delayTime) {
          trashcan.state = OPENING;
          trashcan.stateStartTime = millis();
        }
        break;

      case OPENING:
        if (motionDetected) {
          Serial.println("Motion Detected! Closing Again.");
          trashcan.state = CLOSING;
        } else {
          digitalWrite(LED, HIGH);
          analogWrite(trashcan.trashcanPin, 70);
          trashcan.bottomReached = false;
        }
        if (millis() - trashcan.stateStartTime >= trashcan.openingTime) {
          analogWrite(trashcan.trashcanPin, 0);
          trashcan.state = OPEN;
          trashcan.stateStartTime = millis();
        }
        break;

      case OPEN:
        trashcan.bottomReached = false;
        if (motionDetected) {
          trashcan.state = CLOSING;
          trashcan.stateStartTime = millis();
        }
        break;
    }
  }
}
