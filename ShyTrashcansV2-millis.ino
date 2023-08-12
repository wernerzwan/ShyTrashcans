int TrashcanPin = 3;
int TrashcanReverse = 4;
int LED = 13; // onboard LED pin
volatile bool bottomReached = false;

unsigned long stateStartTime = 0;
unsigned long delayTime = 15000; // Delay time in milliseconds
enum TrashcanState { CLOSING, CLOSED, OPENING, OPEN };

TrashcanState currentState = CLOSING;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT); // LED Pin Output
  pinMode(TrashcanPin, OUTPUT);
  pinMode(TrashcanReverse, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(2), hallSensorInterrupt, HIGH); // Hall sensor interrupt on pin D2
}

void loop() {
  switch (currentState) {
    case CLOSING:
      digitalWrite(LED, LOW); // LED off
      analogWrite(TrashcanPin, 180);
      Serial.println("closing");
      if (bottomReached) {
        bottomReached = false;
        currentState = CLOSED;
        stateStartTime = millis();
      }
      break;

    case CLOSED:
      analogWrite(TrashcanPin, 0);
      Serial.println("closed");
      bottomReached = false;
      if (millis() - stateStartTime >= delayTime) {
        currentState = OPENING;
        stateStartTime = millis();
      }
      break;

    case OPENING:
      digitalWrite(LED, HIGH); // LED on
      analogWrite(TrashcanPin, 70);
      Serial.println("opening");
      if (millis() - stateStartTime >= 2600) {
        analogWrite(TrashcanPin, 0);
        currentState = OPEN;
        stateStartTime = millis();
      }
      break;

    case OPEN:
    Serial.println("open");
      if (millis() - stateStartTime >= delayTime) {
        currentState = CLOSING;
        stateStartTime = millis();
      }
      break;
  }
}

void hallSensorInterrupt() {
  if (currentState == CLOSED || currentState == CLOSING) {
    bottomReached = true;
  }
}
