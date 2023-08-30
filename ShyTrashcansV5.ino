int TrashcanPin1 = 3;
int ReversePin1 = 6;
int TrashcanPin2 = 9;
int ReversePin2 = 10;
int LED = 13; // onboard LED pin
int PIRPin = 5; // PIR sensor pin
const int HALPin1 = 2;
const int HALPin2 = 4;
bool motionDetected = false;
bool bottomReached1 = false;
bool bottomReached2 = false;

unsigned long stateStartTime1 = 0;
unsigned long stateStartTime2 = 0;
unsigned long delayTime1 = 3000; // Delay time in milliseconds
unsigned long delayTime2 = 8000; // Delay time in milliseconds
enum TrashcanState { CLOSING, REVERSE, CLOSED, OPENING, OPEN };

TrashcanState currentState1 = CLOSING;
TrashcanState currentState2 = CLOSING;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT); // LED Pin Output
  pinMode(TrashcanPin1, OUTPUT);
  pinMode(ReversePin1, OUTPUT);
  pinMode(TrashcanPin2, OUTPUT);
  pinMode(ReversePin2, OUTPUT);
  pinMode(PIRPin, INPUT);
  pinMode(HALPin1, INPUT);
  pinMode(HALPin2, INPUT);
}

void loop() {
  // Read motion sensor state

  switch (currentState1) {
    case CLOSING:
    bottomReached1 = digitalRead(HALPin1);
      if (bottomReached1) {
        currentState1 = REVERSE;
        stateStartTime1 = millis();
      }
      else { 
      digitalWrite(LED, LOW); // LED off
      analogWrite(TrashcanPin1, 255);
      }
      break;

    case REVERSE:
      analogWrite(TrashcanPin1, 0);
      if (millis() - stateStartTime1 >= 50) {
        analogWrite(ReversePin1, 100);
      }
      if (millis() - stateStartTime1 >=250) {
        analogWrite(ReversePin1, 0); 
        currentState1 = CLOSED;
        stateStartTime1 = millis();
        Serial.println("Reversing");
      }
      break;

    case CLOSED:
      analogWrite(TrashcanPin1, 0);
      motionDetected = digitalRead(PIRPin);
      if (motionDetected){
        stateStartTime1 = millis();
        Serial.println("Motion Detected! Resetting Timer.");
      }
      Serial.print("Time remaining untill next attempt:");
      Serial.println(delayTime1 + stateStartTime1 - millis());
      if (millis() - stateStartTime1 >= delayTime1) {
        currentState1 = OPENING;
        stateStartTime1 = millis();
      }
      break;

    case OPENING:
    motionDetected = digitalRead(PIRPin);
    if (motionDetected){
              Serial.println("Motion Detected! Closing Again.");
              currentState1 = CLOSING;
    } else{ 
      digitalWrite(LED, HIGH); // LED on
      analogWrite(TrashcanPin1, 70);
      bottomReached1 = false;
    }
          if (millis() - stateStartTime1 >= 2700) {
        analogWrite(TrashcanPin1, 0);
        currentState1 = OPEN;
        stateStartTime1 = millis();
      }
      break;

    case OPEN:
      bottomReached1 = false;
      motionDetected = digitalRead(PIRPin);
      if (motionDetected) {
        currentState1 = CLOSING;
        stateStartTime1 = millis();
      } 
      break;
  }

       switch (currentState2) {
    case CLOSING:
    bottomReached2 = digitalRead(HALPin2);
      if (bottomReached2) {
        currentState2 = REVERSE;
        stateStartTime2 = millis();
      }
      else { 
      digitalWrite(LED, LOW); // LED off
      analogWrite(TrashcanPin2, 255);
      }
      break;

    case REVERSE:
      analogWrite(TrashcanPin2, 0);
      if (millis() - stateStartTime2 >= 50) {
        analogWrite(ReversePin2, 100);
      }
      if (millis() - stateStartTime2 >=250) {
        analogWrite(ReversePin2, 0); 
        currentState2 = CLOSED;
        stateStartTime2 = millis();
        Serial.println("Reversing");
      }
      break;

    case CLOSED:
      analogWrite(TrashcanPin2, 0);
      motionDetected = digitalRead(PIRPin);
      if (motionDetected){
        stateStartTime2 = millis();
        Serial.println("Motion Detected! Resetting Timer.");
      }
      Serial.print("Time remaining untill next attempt:");
      Serial.println(delayTime2 + stateStartTime2 - millis());
      if (millis() - stateStartTime2 >= delayTime2) {
        currentState2 = OPENING;
        stateStartTime2 = millis();
      }
      break;

    case OPENING:
    motionDetected = digitalRead(PIRPin);
    if (motionDetected){
              Serial.println("Motion Detected! Closing Again.");
              currentState2 = CLOSING;
    } else{ 
      digitalWrite(LED, HIGH); // LED on
      analogWrite(TrashcanPin2, 70);
      bottomReached2 = false;
    }
          if (millis() - stateStartTime2 >=2000) {
        analogWrite(TrashcanPin2, 0);
        currentState2 = OPEN;
        stateStartTime2 = millis();
      }
      break;

    case OPEN:
      bottomReached2 = false;
      motionDetected = digitalRead(PIRPin);
      if (motionDetected) {
        currentState2 = CLOSING;
        stateStartTime2 = millis();
      } 
      break;
  }
    
}

