#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

const int numSensors = 2;           // Number of sensors
int transposeAmount = 0;            // Transpose amount in semitones
float alpha1 = 0.02;                 // Smoothing factor for exponential smoothing
float alpha2 = 0.01;                 // Smoothing factor for exponential smoothing
float smoothedValue[numSensors] = {0.0, 0.0}; // Smoothed sensor readings for 2 sensors
int lastTransposeAmount = 1;
const char* keys[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char* scaleNames[] = {"Minor Pentatonic", "Blues", "Natural Minor", "Harmonic Minor","Stairway to Hell","Diablo Shift", "Major Pentatonic", "Major", "Melodic Minor Desc", "Dorian", "Mixolydian", "Phrygian"};

int cMinorPentatonic[8] = {60, 60, 63, 65, 67, 70, 70, 72};    // C Eb F G Bb C
int cBlues[8] = {60, 63, 65, 66, 67, 70, 70, 72};              // C Eb F F# G Bb C
int cNaturalMinor[8] = {60, 62, 63, 65, 67, 68, 70, 72};       // C D Eb F G Ab Bb C
int cHarmonicMinor[8] = {60, 62, 63, 65, 67, 68, 71, 72};      // C D Eb F G Ab B C
int cDiablo1[8] = {60, 63, 66, 69, 72, 75, 78, 81};            // C Eb F# A C D# F# A
int cDiablo2[8] = {60, 61, 67, 68, 72, 73, 75, 76};            // C C# G G# C C# D# E
int cMajorPentatonic[8] = {60, 60, 62, 64, 67, 69, 69, 72};    // C D E G A C
int cMajor[8] = {60, 62, 64, 65, 67, 69, 71, 72};              // C D E F G A B C
int cMelodicMinorDesc[8] = {60, 62, 63, 65, 67, 68, 70, 72};   // C D Eb F G Ab Bb C
int cDorian[8] = {60, 62, 63, 65, 67, 69, 70, 72};             // C D Eb F G A Bb C
int cMixolydian[8] = {60, 62, 64, 65, 67, 69, 70, 72};         // C D E F G A Bb C
int cPhrygian[8] = {60, 61, 63, 65, 67, 68, 70, 72};           // C Db Eb F G Ab Bb C

int* scales[] = {cMinorPentatonic, cBlues, cNaturalMinor, cHarmonicMinor, cDiablo1, cDiablo2, cMajorPentatonic, cMajor,cMelodicMinorDesc, cDorian, cMixolydian, cPhrygian};
int currentScale = 0;
int lastScale = 0;

const int NumNotes = 8;  
const int decreaseButtonPin = 2; // Pin D2
const int increaseButtonPin = 3; // Pin D3
const int scaleButtonPin = 4;    // Pin D4
int lastIncreaseState = LOW;
int lastDecreaseState = LOW;
int lastScaleState = LOW;

const unsigned long holdDuration = 200; // Time (in ms) to differentiate between a press and a hold
const unsigned long debounceDelay = 50;  // Debounce delay

unsigned long increasePressTime = 0;
unsigned long decreasePressTime = 0;

bool increaseActionTaken = false; 
bool decreaseActionTaken = false;

const int ledPin1 = 10;         // Pin D10
const int ledPin2 = 11;         // Pin D11

// For the display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for all other sizes
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Apply exponential smoothing to sensor readings
void applyExponentialSmoothing(int sensorIndex, float alphaValue) {
  int rawReading = analogRead(sensorIndex);
  smoothedValue[sensorIndex] = alphaValue * (rawReading - smoothedValue[sensorIndex]) + smoothedValue[sensorIndex];
}

// Handle button presses to update transposeAmount or the currentScale
void handleButtonPresses() {
  int increaseState = digitalRead(increaseButtonPin);
  int decreaseState = digitalRead(decreaseButtonPin);
  int scaleState = digitalRead(scaleButtonPin);

  if (increaseState == LOW && lastIncreaseState == HIGH) {
    // button just pressed
    increasePressTime = millis();
    increaseActionTaken = false;
  } else if (increaseState == HIGH && lastIncreaseState == LOW && !increaseActionTaken) {
    // button just released and action not taken yet
    unsigned long pressDuration = millis() - increasePressTime;
    if (pressDuration < debounceDelay) {
      // Ignore this reading
      return;
    }
    if (pressDuration > holdDuration) {
      transposeAmount += 12; // Shift up by one octave
    } else {
      transposeAmount++;     // Shift up by one halfstep
    }
    increaseActionTaken = true; // Mark action as taken
  }

  if (decreaseState == LOW && lastDecreaseState == HIGH) {
    // button just pressed
    decreasePressTime = millis();
    decreaseActionTaken = false;
  } else if (decreaseState == HIGH && lastDecreaseState == LOW && !decreaseActionTaken) {
    // button just released and action not taken yet
    unsigned long pressDuration = millis() - decreasePressTime;
    if (pressDuration < debounceDelay) {
      // Ignore this reading
      return;
    }
    if (pressDuration > holdDuration) {
      transposeAmount -= 12; // Shift down by one octave
    } else {
      transposeAmount--;     // Shift down by one halfstep
    }
    decreaseActionTaken = true; // Mark action as taken
  }

  if (scaleState == LOW && lastScaleState == HIGH) {
    currentScale = (currentScale + 1) % 12; // assuming we have 12 scales
    delay(100); //simple debounce
  }

  lastIncreaseState = increaseState;
  lastDecreaseState = decreaseState;
  lastScaleState = scaleState;
}




void updateDisplay() {
  display.clearDisplay();

  // Displaying the key
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,12.5); // Screen coordinates for where the text should be displayed
  display.print(F("Key: "));
  int index = (transposeAmount % 12 + 12) % 12;  // Calculate the index of the keys array
  display.setTextSize(2);
  display.setCursor(40,10);
  display.println(keys[index]);

  // Displaying the current scale
  display.setTextSize(1); 
  display.setCursor(0,0); 
  display.println(scaleNames[currentScale]);

  // Displaying the midi root note
  display.setTextSize(1); 
  display.setCursor(0,25); 
  display.print(F("MIDI root note: "));
  display.println(60+transposeAmount);
  
  display.display();
}

// Convert sensor values to MIDI notes using custom segments
int sensorValueToMidiScale_CustomSegments(int num) {
  if (num >= 0 && num <= 1000) {
    int segments[8][2] = {
      {0, 187},             
      {188, 249},
      {250, 299},
      {300, 349},
      {350, 399},
      {400, 459},
      {460, 527},
      {528, 1000}
    };
    //int midiNotes[7] = {60, 60, 63, 65, 67, 70, 72};  // Corresponding MIDI note numbers
    //int midiNotes[7] = {60, 61, 63, 65, 67, 68, 72};  // Corresponding MIDI note numbers
    int* midiNotes = scales[currentScale];

    for (int i = 0; i < 8; ++i) {
      if (num >= segments[i][0] && num <= segments[i][1]) {
        return midiNotes[i] + transposeAmount;
      }
    }
  }
  return -1;  // Return a special value to indicate an error
}

// Convert sensor values to MIDI notes using equal segments
int sensorValueToMidiScale_EqualSegments(int num, int start_range = 180, int end_range = 480) {
  int segment_length = (end_range - start_range) / NumNotes;
  int* midiNotes = scales[currentScale];
  
  for (int i = 0; i < NumNotes; i++) {
    int start_segment = start_range + segment_length * i;
    int end_segment = start_range + segment_length * (i + 1) - 1;
    if (start_segment <= num && num <= end_segment) {
      return midiNotes[i] + transposeAmount;
    }
  }

  if (abs(num - start_range) < abs(num - end_range)) {
    return midiNotes[0] + transposeAmount;
  } else {
    return midiNotes[NumNotes - 1] + transposeAmount;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(increaseButtonPin, INPUT_PULLUP); // activate the internal pull-up resistor for the buttons
  pinMode(decreaseButtonPin, INPUT_PULLUP);
  pinMode(scaleButtonPin, INPUT_PULLUP);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  delay(2000); // delay for OLED to initialize before writing text:

  // Clear the buffer
  display.clearDisplay();
  display.display();
}

void loop() {
  // Handle button presses for transposing notes
  handleButtonPresses();

  if (transposeAmount != lastTransposeAmount || currentScale != lastScale) {
    // Call the updateDisplay function to update the OLED Display when transposeAmount or currentScale changes
    updateDisplay();
    
    lastTransposeAmount = transposeAmount; // Update lastTransposeAmount to the current transposeAmount
    lastScale = currentScale; // Update lastScale to the currentScale
  }

for (int i = 0; i < numSensors; i++) {
    // Smooth the sensor readings
    applyExponentialSmoothing(i, (i == 0) ? alpha1 : alpha2); //If i = 0, then choose alpha1. Otherwise, choose alpha2
    int ledBrightness = map((int)smoothedValue[i], 200, 450, 0, 255);
    ledBrightness = constrain(ledBrightness, 0, 255);
    
    if (i == 0) {
    // For sensor 2 (if sensor index is 1), just print the smoothed sensor value
      //int midiNote = sensorValueToMidiScale_CustomSegments((int)smoothedValue[i]);
      int midiNote = sensorValueToMidiScale_EqualSegments((int)smoothedValue[i]);
      Serial.print(midiNote);
      Serial.print(" ");
      analogWrite(ledPin1, ledBrightness);
    } else {
    // For sensor 1 (If the sensor index is 0), convert the smoothed sensor value to a MIDI note
      Serial.print((int)smoothedValue[i]);
      Serial.print(" "); 
      analogWrite(ledPin2, ledBrightness);
    }
}
  Serial.println();
}

