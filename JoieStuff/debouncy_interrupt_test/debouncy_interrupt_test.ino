const int led = 13;
const int redoPin = 12;
const int nextPin = 11;
boolean ledOn = false;

const int debounceDelay = 95;  // Debounce delay in milliseconds
const unsigned long longPressThreshold = 3000;  // 3 seconds threshold

bool nextPressed = false;
bool redoPressed = false;
bool nextButtonHeld = false;

unsigned long nextLastPressTime = 0;
unsigned long redoLastPressTime = 0;
unsigned long nextButtonPressStartTime = 0;

bool nextWasHeld = false;

void setup() {
  pinMode(led, OUTPUT);
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(redoPin, INPUT_PULLUP);

  // Attach interrupts to the buttons
  attachInterrupt(digitalPinToInterrupt(nextPin), nextButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(redoPin), redoButtonISR, FALLING);
}

void loop() {

  checkNextHold();

  // Check if the next button was pressed
  if (nextPressed) {
    nextPressed = false;

    Serial.println("NextButtonPressed");
    ledOn = !ledOn;  // Toggle the LED state
  }

  // Check if the redo button was pressed
  if (redoPressed) {
    redoPressed = false;

    Serial.println("RedoPressed");
    ledOn = !ledOn;  // Toggle the LED state
  }

  if (nextButtonHeld) {
    nextButtonHeld=false;
    
    Serial.println("NextButtonHeld");
    ledOn = !ledOn;  // For example, turn the LED on if held for long
  }

  digitalWrite(led, ledOn);  // Update the LED state
}

// Interrupt Service Routine for the Next button
void nextButtonISR() {
  // Check if enough time has passed since the last press
  if (millis() - nextLastPressTime > debounceDelay) {
    if(nextWasHeld){
      nextWasHeld=false;
    }
    else{
      nextPressed = true;
      nextLastPressTime = millis();  // Update the last press time
    }
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

void checkNextHold(){
  // Check if the next button has been held for more than 3 seconds
  bool pinVal = digitalRead(nextPin);
  if (pinVal==HIGH && (millis() - nextButtonPressStartTime) > longPressThreshold && nextWasHeld==false){
    nextButtonHeld = true;
    nextWasHeld = true;
    delay(50);
  }
  else if (pinVal == LOW){
    nextButtonPressStartTime = millis();
  }
}

