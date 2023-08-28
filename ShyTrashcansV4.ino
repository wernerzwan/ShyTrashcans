int TrashcanPin = 3;
int LED = 13; // onboard LED pin
int PIRPin = 5; // PIR sensor pin
int HALPin = 2;
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
  pinMode(HALPin, INPUT);
}

void loop() {
  // Read motion sensor state
  Serial.println(digitalRead(HALPin));

  switch (currentState) {
    case CLOSING:
    bottomReached = digitalRead(HALPin);
      if (bottomReached) {
        currentState = CLOSED;
        stateStartTime = millis();
        Serial.println("bottom reached");
      }
      else { 
      digitalWrite(LED, LOW); // LED off
      analogWrite(TrashcanPin, 255);
      }
      break;

    case CLOSED:
      analogWrite(TrashcanPin, 0);
      motionDetected = digitalRead(PIRPin);
      if (motionDetected){
        stateStartTime = millis();
        Serial.println("Motion Detected! Resetting Timer.");
      }
      Serial.print("Time remaining untill next attempt:");
      Serial.println(delayTime + stateStartTime - millis());
      Serial.print(bottomReached, DEC);
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
          if (millis() - stateStartTime >= 800) {
        analogWrite(TrashcanPin, 0);
        currentState = OPEN;
        stateStartTime = millis();
      }
      break;

    case OPEN:
      bottomReached = false;
      motionDetected = digitalRead(PIRPin);
      if (motionDetected) {
        currentState = CLOSING;
        stateStartTime = millis();
      } 
      break;
  }
}

