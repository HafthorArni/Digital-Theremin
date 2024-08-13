#include <Arduino.h>
#include <Adafruit_SSD1306.h>
const int numSensors = 2;           // Number of sensors
int transposeAmount = 0;            // Transpose amount in semitones
float alpha = 0.01;                 // Smoothing factor for exponential smoothing
float smoothedValue[numSensors] = {0.0, 0.0}; // Smoothed sensor readings for 2 sensors

const int NumNotes = 5;  
const int decreaseButtonPin = 2;
const int increaseButtonPin = 3;
int lastIncreaseState = LOW;
int lastDecreaseState = LOW;

const int ledPin1 = 10; 
const int ledPin2 = 11;

// Apply exponential smoothing to sensor readings
void applyExponentialSmoothing(int sensorIndex) {
  int rawReading = analogRead(sensorIndex);
  smoothedValue[sensorIndex] = alpha * (rawReading - smoothedValue[sensorIndex]) + smoothedValue[sensorIndex];
}

// Handle button presses to update transposeAmount
void handleButtonPresses() {
  int increaseState = digitalRead(increaseButtonPin);
  int decreaseState = digitalRead(decreaseButtonPin);

  if (increaseState == HIGH && lastIncreaseState == LOW) {
    transposeAmount++;
  }
  if (decreaseState == HIGH && lastDecreaseState == LOW) {
    transposeAmount--;
  }

  lastIncreaseState = increaseState;
  lastDecreaseState = decreaseState;
}

// Convert sensor values to MIDI notes using custom segments
int sensorValueToMidiScale_CustomSegments(int num) {
  if (num >= 0 && num <= 1000) {
    int segments[7][2] = {
      {0, 187},             
      {188, 200},
      {201, 299},
      {300, 391},
      {392, 459},
      {460, 527},
      {528, 1000}
    };
    //int midiNotes[7] = {60, 60, 63, 65, 67, 70, 72};  // Corresponding MIDI note numbers
    int midiNotes[7] = {60, 61, 63, 65, 67, 68, 72};  // Corresponding MIDI note numbers
    for (int i = 0; i < 7; ++i) {
      if (num >= segments[i][0] && num <= segments[i][1]) {
        return midiNotes[i] + transposeAmount;
      }
    }
  }
  return -1;  // Return a special value to indicate an error
}

// Convert sensor values to MIDI notes using equal segments
int sensorValueToMidiScale_EqualSegments(int num, int start_range = 120, int end_range = 590) {
  int segment_length = (end_range - start_range) / NumNotes;
  int midi_notes[] = {71, 69, 64, 62, 59}; // C minor pentatonic
  
  for (int i = 0; i < NumNotes; i++) {
    int start_segment = start_range + segment_length * i;
    int end_segment = start_range + segment_length * (i + 1) - 1;
    if (start_segment <= num && num <= end_segment) {
      return midi_notes[i] + transposeAmount;
    }
  }

  if (abs(num - start_range) < abs(num - end_range)) {
    return midi_notes[0] + transposeAmount;
  } else {
    return midi_notes[NumNotes - 1] + transposeAmount;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(increaseButtonPin, INPUT_PULLUP);
  pinMode(decreaseButtonPin, INPUT_PULLUP);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
}

void loop() {
  // Handle button presses for transposing notes
  handleButtonPresses();

  for (int i = 0; i < numSensors; i++) {
    
    // Smooth the sensor readings
    applyExponentialSmoothing(i);
    int ledBrightness = map((int)smoothedValue[i], 200, 450, 0, 255);
    ledBrightness = constrain(ledBrightness, 0, 255);
    // If the sensor index is 0, convert the smoothed sensor value to a MIDI note
    if (i == 0) {
      int midiNote = sensorValueToMidiScale_CustomSegments((int)smoothedValue[i]);
      Serial.print(midiNote);
      Serial.print(" ");
      analogWrite(ledPin1, ledBrightness);
    } else {
      // Otherwise, just print the smoothed sensor value
      Serial.print((int)smoothedValue[i]);
      Serial.print(" "); 
      analogWrite(ledPin2, ledBrightness);
    }
  }
  Serial.println();
}

