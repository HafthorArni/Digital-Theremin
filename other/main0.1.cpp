#include <Arduino.h>

const int numReadings = 100;       // Number of readings to average over
int readings[2][numReadings];     // 2D array to hold last N readings for 2 sensors
int index = 0;                    // Index of the current reading
long total[2] = {0, 0};           // Running total for 2 sensors
int average[2] = {0, 0};          // Average for 2 sensors
const int NumNotes = 4;           // Number of notes
int transposeAmount = 0;          // Transpose amount in semitones

// Smoothing function to get and smooth sensor readings
void smoothReading(int sensorIndex) {
  // Subtract the last reading
  total[sensorIndex] -= readings[sensorIndex][index];
  // Read from the sensor
  readings[sensorIndex][index] = analogRead(sensorIndex);
  // Add the new reading to the total
  total[sensorIndex] += readings[sensorIndex][index];
  // Calculate the new average
  average[sensorIndex] = total[sensorIndex] / numReadings;
}

// Function to convert an input number to a MIDI note
int getMidiNoteFromNumber(int num, int start_range = 120, int end_range = 590) {
  int segment_length = (end_range - start_range) / NumNotes;
  int midi_notes[] = {71, 68, 62, 59}; // C minor pentatonic

  // Determine which MIDI note corresponds to the input number
  for (int i = 0; i < NumNotes; i++) {
    int start_segment = start_range + segment_length * i;
    int end_segment = start_range + segment_length * (i + 1) - 1;
    if (start_segment <= num && num <= end_segment) {
      return midi_notes[i] + transposeAmount;
    }
  }

  // If number is outside range, return the closest note
  if (abs(num - start_range) < abs(num - end_range)) {
    return midi_notes[0] + transposeAmount;
  } else {
    return midi_notes[NumNotes - 1] + transposeAmount;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize all readings to 0
  for (int i = 0; i < numReadings; i++) {
    readings[0][i] = 0;
    readings[1][i] = 0;
  }
}

void loop() {
  for (int i = 0; i < 2; i++) {
    // Perform smoothing for each sensor
    smoothReading(i);
    
    // For the first sensor, apply the getMidiNoteFromNumber function
    if (i == 0) {
      average[i] = getMidiNoteFromNumber(average[i]);
    }

    // Print out the average value
    Serial.print(average[i]);
    Serial.print(" ");
  }

  // Update index for the next reading
  index = (index + 1) % numReadings;

  Serial.println();
}
