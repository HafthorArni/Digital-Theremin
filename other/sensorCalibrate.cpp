#include <Arduino.h>
float smoothedValue = 0.0;
float alpha = 0.01;

int sensorPin = 0;
int buttonPin = 2;

void setup() {
  Serial.begin(115200);
  pinMode(2, INPUT_PULLUP);  // Enable internal pull-up
}

void applyExponentialSmoothing(int sensorPin) {
  int rawReading = analogRead(sensorPin);
  smoothedValue = alpha * (rawReading - smoothedValue) + smoothedValue;
}

void loop(){
  int press = digitalRead(buttonPin);
  applyExponentialSmoothing(sensorPin);
  if (press == LOW){  // Button press will now read as LOW
    Serial.println(smoothedValue);
  }
}
