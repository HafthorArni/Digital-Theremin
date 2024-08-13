#include <Arduino.h>

const int numReadings = 5;       // Number of readings to average over
int readings[2][numReadings];     // 2D array to hold last N readings for 2 sensors
int index = 0;                    // Index of the current reading
long total[2] = {0, 0};           // Running total for 2 sensors
int average[2] = {0, 0};          // Average for 2 sensors
const int NumNotes = 5;           // Number of notes 
int transposeAmount = 0;          // Transpose amount in semitones
float alpha = 0.01;                // Smoothing factor for exponential smoothing
float smoothedValue[2] = {0.0, 0.0}; // Smoothed sensor readings for 2 sensors

const int decreaseButtonPin = 2;
const int increaseButtonPin = 3;
int lastIncreaseState = LOW;
int lastDecreaseState = LOW;

// Function for exponential smoothing
void exponentialSmoothing(int sensorIndex) {
  int rawReading = analogRead(sensorIndex);
  smoothedValue[sensorIndex] = alpha * (rawReading - smoothedValue[sensorIndex]) + smoothedValue[sensorIndex];
  average[sensorIndex] = (int) smoothedValue[sensorIndex];  // Update to the latest smoothed value
}

// Function for averaging filter
void averageSmoothing(int sensorIndex) {
  // Subtract the last reading
  total[sensorIndex] -= readings[sensorIndex][index];
  // Read from the sensor
  readings[sensorIndex][index] = analogRead(sensorIndex);
  // Add the new reading to the total
  total[sensorIndex] += readings[sensorIndex][index];
  // Calculate the new average
  average[sensorIndex] = total[sensorIndex] / numReadings;
}

// Use this Function to convert an input number to a MIDI note 
int getMidiNoteFromNumber_EqualSegments(int num, int start_range = 120, int end_range = 590) {
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

// This function is 
int getMidiNoteFromNumber_CustomSegments(int num) {
    if (num >= 0 && num <= 1000) {
        int segments[7][2] = {
            {0, 187},  // C4
            {188, 200},  // D4
            {201, 323},  // E4
            {324, 391},  // F4
            {392, 459},  // G4
            {460, 527},  // A4
            {528, 1000}   // B4
        };
        int midi_notes[7] = {60, 62, 63, 65, 67, 70, 72};  // Corresponding MIDI note numbers
        
        for (int i = 0; i < 7; ++i) {
            if (num >= segments[i][0] && num <= segments[i][1]) {
                return midi_notes[i] + transposeAmount;
            }
        }
    }
    return -1;  // Return a special value to indicate an error
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < numReadings; i++) {
    readings[0][i] = 0;
    readings[1][i] = 0;
  }

  pinMode(increaseButtonPin, INPUT_PULLUP);
  pinMode(decreaseButtonPin, INPUT_PULLUP);
}

void loop() {
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

  for (int i = 0; i < 2; i++) {
    // Uncomment the line below to use the averaging filter
    //averageSmoothing(i);

    // Uncomment the line below to use the exponential smoothing filter
    exponentialSmoothing(i);

    // if (i == 0) {
    //   average[i] = getMidiNoteFromNumber_EqualSegments(average[i]);
    // }

    if (i == 0) {
      average[i] = getMidiNoteFromNumber_CustomSegments(average[i]);
    }

    Serial.print(average[i]);
    Serial.print(" ");
  }
  // Update index for the next reading
  index = (index + 1) % numReadings;
  Serial.println();
}
