int HallSensor = 2; // Hall sensor is connected to the D2 pin
int TrashcanPin = 3;
int TrashcanReverse = 4;
int LED = 13; // onboard LED pin

void setup() {

  Serial.begin(9600);
  pinMode(HallSensor, INPUT); // Hall Effect Sensor pin INPUT
  pinMode(LED, OUTPUT); // LED Pin Output
  pinMode(TrashcanPin, OUTPUT);
  pinMode(TrashcanReverse, OUTPUT);
}


void loop() {

  
  int sensorStatus = digitalRead(HallSensor); // Check the sensor status
  if (sensorStatus == HIGH) // Check if the pin high or not
    {
    // if the pin is high turn on the onboard Led
    digitalWrite(LED, HIGH); // LED on
    digitalWrite(TrashcanPin, LOW);
    // digitalWrite(TrashcanReverse, HIGH);
    Serial.println("bottom reached");
    // delay(10);
    digitalWrite(TrashcanReverse, LOW);
    delay(15000);
    Serial.println("moving up");
    analogWrite(TrashcanPin, 70);
    delay(3000);
    digitalWrite(TrashcanPin,LOW);
    Serial.println("up reached");
    delay(15000);
  } else  {
    //else turn off the onboard LED
    Serial.println("closing down");
    digitalWrite(LED, LOW); // LED off
    digitalWrite(TrashcanReverse, LOW);
    analogWrite(TrashcanPin, 180);
  }
}

