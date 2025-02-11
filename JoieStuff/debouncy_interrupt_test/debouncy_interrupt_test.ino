const int led = 13;
const int redoPin = 12;
const int nextPin = 11;
boolean ledOn = false;

const int debounceDelay = 85;  // Debounce delay in milliseconds

bool nextPressed = false;
bool redoPressed = false;

unsigned long nextLastPressTime = 0;
unsigned long redoLastPressTime = 0;

void setup() {
  pinMode(led, OUTPUT);
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(redoPin, INPUT_PULLUP);

  // Attach interrupts to the buttons
  attachInterrupt(digitalPinToInterrupt(nextPin), nextButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(redoPin), redoButtonISR, FALLING);
}

void loop() {
  // Check if the next button was pressed
  if (nextPressed) {
    nextPressed = false;
    ledOn = !ledOn;  // Toggle the LED state
  }

  // Check if the redo button was pressed
  if (redoPressed) {
    redoPressed = false;
    ledOn = !ledOn;  // Toggle the LED state
  }

  digitalWrite(led, ledOn);  // Update the LED state
}

// Interrupt Service Routine for the Next button
void nextButtonISR() {
  // Check if enough time has passed since the last press
  if (millis() - nextLastPressTime > debounceDelay) {
    nextPressed = true;
    nextLastPressTime = millis();  // Update the last press time
  }
}

// Interrupt Service Routine for the Redo button
void redoButtonISR() {
  // Check if enough time has passed since the last press
  if (millis() - redoLastPressTime > debounceDelay) {
    redoPressed = true;
    redoLastPressTime = millis();  // Update the last press time
  }
}
