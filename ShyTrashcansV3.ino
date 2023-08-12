int TrashcanPin = 3;
int LED = 13; // onboard LED pin
int PIRPin = 5; // PIR sensor pin
volatile bool bottomReached = false;
bool motionDetected = false;

unsigned long stateStartTime = 0;
unsigned long delayTime = 15000; // Delay time in milliseconds
enum TrashcanState { CLOSING, CLOSED, OPENING, OPEN };

TrashcanState currentState = CLOSING;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT); // LED Pin Output
  pinMode(TrashcanPin, OUTPUT);
  pinMode(PIRPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), hallSensorInterrupt, HIGH); // Hall sensor interrupt on pin D2
}

void loop() {
  // Read motion sensor state
  

  switch (currentState) {
    case CLOSING:
      digitalWrite(LED, LOW); // LED off
      analogWrite(TrashcanPin, 255);
      if (bottomReached) {
        bottomReached = false;
        currentState = CLOSED;
        stateStartTime = millis();
      }
      break;

    case CLOSED:
      analogWrite(TrashcanPin, 0);
      motionDetected =digitalRead(PIRPin);
      if (motionDetected){
        stateStartTime = millis();
        Serial.println("Motion Detected! Resetting Timer.");
      }
      Serial.print("Time remaining untill next attempt:");
      Serial.println(delayTime + stateStartTime - millis());
      if (millis() - stateStartTime >= delayTime) {
        currentState = OPENING;
        stateStartTime = millis();
      }
      break;

    case OPENING:
    motionDetected = digitalRead(PIRPin);
    if (motionDetected){
              Serial.println("Motion Detected! Closing Again.");
              currentState = CLOSING;
    } else{ 
      digitalWrite(LED, HIGH); // LED on
      analogWrite(TrashcanPin, 70);
      bottomReached = false;
    }
          if (millis() - stateStartTime >= 2500) {
        analogWrite(TrashcanPin, 0);
        currentState = OPEN;
        stateStartTime = millis();
      }
      break;

    case OPEN:
      motionDetected = digitalRead(PIRPin);
      if (motionDetected) {
        currentState = CLOSING;
        stateStartTime = millis();
      } else {
        currentState = OPEN;
      }
      break;
  }
}

void hallSensorInterrupt() {
    bottomReached = true;
}
