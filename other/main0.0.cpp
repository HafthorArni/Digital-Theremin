#include <Arduino.h>

const int numReadings = 50;   // the number of readings to average over
int readings[2][numReadings]; // 2D array to hold the last N readings for 2 sensors
int index = 0;                // the index of the current reading
long total[2] = {0, 0};       // the running total for 2 sensors
int average[2] = {0, 0};      // the average for 2 sensors
int NumNotes = 4;

int transposeAmount = 0; // Set this to the desired number of semitones to transpose. Positive values transpose up, negative values transpose down.

int getMidiNoteFromNumber(int num, int start_range = 120, int end_range = 590) {
  int segment_length = (end_range - start_range) / NumNotes;
  //int midi_notes[] = {72, 70, 67, 65, 63, 60}; // C minor pentatonic
  int midi_notes[] = {71, 68, 62, 59}; // C minor pentatonic
  for (int i = 0; i < NumNotes; i++) {
    int start_segment = start_range + segment_length * i;
    int end_segment = start_range + segment_length * (i + 1) - 1;
    if (start_segment <= num && num <= end_segment) {
      return midi_notes[i] + transposeAmount;
    }
  }

  // If the code reaches here, then the number is outside the range
  if (abs(num - start_range) < abs(num - end_range)) {
    return midi_notes[0] + transposeAmount; // Return the first MIDI note which is closest to the start of the range
  } else {
    return midi_notes[NumNotes-1] + transposeAmount; // Return the last MIDI note which is closest to the end of the range
  }
}

void setup() {
  Serial.begin(115200);
  // initialize all the readings to 0
  for (int i = 0; i < numReadings; i++) {
    readings[0][i] = 0;
    readings[1][i] = 0;
  }
}

void loop() {
  for(int i = 0; i < 2; i++){
    // subtract the last reading
    total[i] -= readings[i][index];
    // read from the sensor
    readings[i][index] = analogRead(i);
    // add the reading to the total
    total[i] += readings[i][index];
    // calculate the average
    average[i] = total[i] / numReadings;
    
    // apply the getMidiNoteFromNumber function for the first sensor
    if(i == 0) {
      average[i] = getMidiNoteFromNumber(average[i]);
    }

    // print out the average value
    Serial.print(average[i]);
    Serial.print(" ");
  }
  // move to the next position in the array
  index = (index + 1) % numReadings;

  Serial.println();
  //delay(1);
}