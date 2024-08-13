#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

const int numSensors = 2;           // Number of sensors
int transposeAmount = 0;            // Transpose amount in semitones
float alpha = 0.01;                 // Smoothing factor for exponential smoothing
float smoothedValue[numSensors] = {0.0, 0.0}; // Smoothed sensor readings for 2 sensors
int lastTransposeAmount = 1;
const char* keys[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

const int NumNotes = 5;  
const int decreaseButtonPin = 2;
const int increaseButtonPin = 3;
int lastIncreaseState = LOW;
int lastDecreaseState = LOW;

const int ledPin1 = 10; 
const int ledPin2 = 11;

// for the display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for all other sizes
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  delay(2000);
  // Clear the buffer
  display.clearDisplay();
  display.display();
}

void loop() {
  // Handle button presses for transposing notes
  handleButtonPresses();

  if (transposeAmount != lastTransposeAmount) {
    // Only update the OLED Display when transposeAmount changes
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print(F("  Key: "));

    // Calculate the index of the keys array by taking transposeAmount modulo 12
    // to ensure that it wraps around correctly, and handle negative transposeAmounts.
    int index = (transposeAmount % 12 + 12) % 12; 

    display.println(keys[index]);
    display.display();

    lastTransposeAmount = transposeAmount; // Update lastTransposeAmount to the current transposeAmount
  }

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

