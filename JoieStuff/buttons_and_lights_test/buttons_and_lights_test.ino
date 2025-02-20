const int dataPin = A1; //shiftRegister pin 14 (red wire)
const int enablePin = A2; //shiftRegister pin 13 (orange wire)
const int latchPin = A3; //shiftRegister pin 11(purple wire)
const int clockPin = A4; //shiftRegister pin 12 (blue wire)

const int redoPin = 12;
const int nextPin = 11;

const int debounceDelay = 95;  // Debounce delay in milliseconds
const unsigned long longPressThreshold = 3000;  // 3 seconds threshold

bool nextPressed = false;
bool redoPressed = false;
bool nextButtonHeld = false;

bool nextWasHeld = false;

unsigned long nextLastPressTime = 0;
unsigned long redoLastPressTime = 0;
unsigned long nextButtonPressStartTime = 0;

uint16_t prevLightState = 0b0000000000000000;
unsigned long lastBlinkTime = 0;
const int blinkInterval = 800;
bool blinkLEDOn = false;

int PEFRTrial = 0;
int PEFRAttempt = 1;
bool inPEFRMode = false;


// Number of LEDs (RGB LEDs)
int numLEDs = 4; 

//BOOL FOR TOGGLING WHICH LED MODE TO TEST
bool testMedMode=false;

void setup() {
  //LED pins
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  //Button pins
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(redoPin, INPUT_PULLUP);

  // Attach interrupts to the buttons
  attachInterrupt(digitalPinToInterrupt(nextPin), nextButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(redoPin), redoButtonISR, FALLING);

  allLEDsOff();
  digitalWrite(enablePin, HIGH);

}

void loop() {
  checkNextHold();

  if (inPEFRMode == false && nextButtonHeld == true){
    nextButtonHeld = false;
    inPEFRMode = true;
  }
  else if (inPEFRMode == true && PEFRTrial == 3 && nextPressed == true){
    nextPressed = false;
    Serial.println("PEFR TEST COMPLETE---------------------");

    //Reset
    setColor("blue", 3);
    delay(1500);

    allLEDsOff();
    inPEFRMode = false;
    PEFRTrial = 0;
    PEFRAttempt = 1;
  }

  if (inPEFRMode == true){
    lightUpPEFRLEDs();
    if(nextPressed == true){
      nextPressed = false;

      PEFRTrial += 1; //move onto the next trial
      PEFRAttempt = 1; //reset attempts
    }
    else if (redoPressed == true){
      redoPressed = false; //reset the redoPressed flag


      redoLights();
      PEFRAttempt += 1;
    }

    Serial.print(F("TRIAL "));
    Serial.print(String(PEFRTrial+1));
    Serial.print(F(", ATTEMPT: "));
    Serial.print(PEFRAttempt);
    Serial.println();
  }
  else if (testMedMode == true){
    //testing inhalation stuff
    for (int i=100; i<600; i+=50){
      lightUpMedLEDs(i);
      delay(100);
    }
    for (int i=600; i>=100; i-=50){
      lightUpMedLEDs(i);
      delay(100);
    }
    allLEDsOff();
  }

  delay(100);
}

//FUNCTION=========================================================================
// Color mapping function
void setColor(String color, int ledIndex) {
  byte red = 0, green = 0, blue = 0;

  // Map color name to RGB values
  if (color == "red") {
    red = 1;
    green = 0;
    blue = 0;
  } else if (color == "green") {
    red = 0;
    green = 1;
    blue = 0;
  } else if (color == "blue") {
    red = 0;
    green = 0;
    blue = 1;
  } else if (color == "yellow") {
    red = 1;
    green = 1;
    blue = 0;
  } else if (color == "off") {
    red = 0;
    green = 0;
    blue = 0;
  }

  // Send color data to the correct LED
  setLEDColor(ledIndex, red, green, blue, prevLightState);
}

// Function to send the color to a specific LED
void setLEDColor(int ledIndex, byte red, byte green, byte blue, uint16_t &prevLightState) {
  uint16_t newLightState;
  if(red == 1){
    newLightState = bitSet(prevLightState, ledIndex*3);
  }
  else if(red == 0){
    newLightState = bitClear(prevLightState, ledIndex*3);
  }

  if(green == 1){
    newLightState = bitSet(prevLightState, ledIndex*3+2);
  }
  else if(green == 0){
    newLightState = bitClear(prevLightState, ledIndex*3+2);
  }

  if(blue == 1){
    newLightState = bitSet(prevLightState, ledIndex*3+1);
  }
  else if(blue == 0){
    newLightState = bitClear(prevLightState, ledIndex*3+1);
  }


  digitalWrite(enablePin, LOW);

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, newLightState >> 8);
  shiftOut(dataPin, clockPin, MSBFIRST, newLightState);
  digitalWrite(latchPin, HIGH);

  prevLightState = newLightState;
}

void allLEDsOff(){
  for (int i=0; i<numLEDs; i++){
    setColor("off", i);
  }
  digitalWrite(enablePin, HIGH);
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

void lightUpPEFRLEDs(){
  for (int i=0; i<PEFRTrial; i++){
    setColor("blue", i);
  }

  unsigned long currentBlinkTime = millis();
  
  if(currentBlinkTime - lastBlinkTime > blinkInterval){
    blinkLEDOn = !blinkLEDOn;
    lastBlinkTime = currentBlinkTime;
  }
  if(blinkLEDOn == true){
    setColor("blue", PEFRTrial);
  }
  else{
    setColor("off", PEFRTrial);
  }
}

void redoLights() {
  //Option 2 go red for a second
  /*
  setColor("red", PEFRTrial);
  delay(1000);
  */

  //Option 1 fast blinking
  for (int i=0; i<8; i++){
    setColor("yellow", PEFRTrial);
    delay(100);
    setColor("off", PEFRTrial);
    delay(100);
  }
}

void lightUpMedLEDs(int flowRate){
  //ToDo: Adjust thresholds
  if(flowRate >= 200 && flowRate < 300){
    //first green
    setColor("green", 0);
    setColor("off", 1);
    setColor("off", 2);
    setColor("off", 3);
  }
  else if(flowRate >= 300 && flowRate < 400){
    //second green
    setColor("green", 0);
    setColor("green", 1);
    setColor("off", 2);
    setColor("off", 3);
  }
  else if(flowRate >= 400 && flowRate < 500){
    //yellow
    setColor("green", 0);
    setColor("green", 1);
    setColor("yellow", 2);
    setColor("off", 3);
  }
  else if(flowRate >= 500){
    //red
    setColor("green", 0);
    setColor("green", 1);
    setColor("yellow", 2);
    setColor("red", 3);
  }
  else {
    allLEDsOff();
  }
}


