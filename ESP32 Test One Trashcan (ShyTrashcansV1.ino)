int HallSensor = 22; // Hall sensor is connected to the D2 pin
int TrashcanPin = 13;
int pinStateCurrent   = LOW; // current state of pin
int pinStatePrevious  = LOW; // previous state of pin
bool bottomReached = false;

const int freq = 10000;
const int ledChannel = 0;
const int resolution = 8;

void setup() {

  Serial.begin(9600);
  pinMode(HallSensor, INPUT); // Hall Effect Sensor pin INPUT
  pinMode(TrashcanPin, OUTPUT);
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(TrashcanPin, ledChannel);
}


void loop() {
  bottomReached = digitalRead(HallSensor); // Check the sensor status
  if (bottomReached) // Check if the pin high or not
    {
    ledcWrite(ledChannel, 0);
    // digitalWrite(TrashcanReverse, HIGH);
    Serial.println("bottom reached");
    // delay(10);
    delay(5000);
    ledcWrite(ledChannel, 80);
    delay(2000);
    ledcWrite(ledChannel, 0);
    Serial.println("up reached");
    delay(5000);
  } else  {
    //else turn off the onboard LED
    ledcWrite(ledChannel, 255);
  }
}

